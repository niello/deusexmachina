#include "ModelRendererSinglePassLight.h"

#include <Scene/SceneNode.h>
#include <Scene/SPS.h>
#include <Render/RenderServer.h>
#include <mathlib/sphere.h>

//!!!multipass lighting benefits from scissor rects for light passes.
//Single-light variant also can use clip planes (AABB for Point & frustum for Spot).

namespace Render
{
ImplementRTTI(Render::CModelRendererSinglePassLight, Render::IModelRenderer);
ImplementFactory(Render::CModelRendererSinglePassLight);

//!!!pass const refs! all calculations must be done earlier in scene or externally!
bool CModelRendererSinglePassLight::IsModelLitByLight(Scene::CModel& Model, Scene::CLight& Light)
{
	if (Light.Type == Scene::CLight::Directional) OK;

	// Check whether this light potentially touches the model
	if (!Model.pSPSRecord->pSPSNode->SharesSpaceWith(*Light.pSPSRecord->pSPSNode)) FAIL;

	//!!!GetBox must return cached box, scene updates it!
	bbox3 ModelBox;
	Model.GetBox(ModelBox);

	// Now test spotlight too. In fact spotlight sphere is much less.
	sphere LightSphere(Light.GetNode()->GetWorldPosition(), Light.GetRange());
	if (LightSphere.clipstatus(ModelBox) == Outside) FAIL;

	if (Light.Type == Scene::CLight::Spot)
	{
		//!!!precalculate once!
		matrix44 LightFrustum;
		Light.CalcFrustum(LightFrustum);
		if (ModelBox.clipstatus(LightFrustum) == Outside) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

//!!!pass const refs! all calculations must be done earlier in scene or externally!
// NB: always returns positive number.
float CModelRendererSinglePassLight::CalcLightPriority(Scene::CModel& Model, Scene::CLight& Light)
{
	// I probably should test for a closest point on or inside bbox, but for now only position is used.
	// Light shining from the center of the object doesn't affect it or does it wrong.

	float SqIntensity = Light.Intensity * Light.Intensity;
	if (Light.Type == Scene::CLight::Directional) return SqIntensity;

	float SqDistance = vector3::SqDistance(Model.GetNode()->GetWorldPosition(), Light.GetNode()->GetWorldPosition());
	float Attenuation = (1.f - SqDistance * (Light.GetInvRange() * Light.GetInvRange()));

	if (Light.Type == Scene::CLight::Spot && SqDistance != 0.f)
	{
		vector3 ModelLight = Model.GetNode()->GetWorldPosition() - Light.GetNode()->GetWorldPosition();
		ModelLight /= n_sqrt(SqDistance);
		float CosAlpha = ModelLight.dot(-Light.GetDirection());
		float Falloff = (CosAlpha - Light.GetCosHalfPhi()) / (Light.GetCosHalfTheta() - Light.GetCosHalfPhi());
		return SqIntensity * Attenuation * n_clamp(Falloff, 0.f, 1.f);
	}

	return SqIntensity * Attenuation;
}
//---------------------------------------------------------------------

void CModelRendererSinglePassLight::Render()
{
	if (!Models.Size()) return;

	vector3 EyePos = RenderSrv->GetCameraPosition();
	for (int i = 0; i < Models.Size(); ++i)
	{
		CModelRecord& Rec = Models[i];

		//LIGHTS+
		for (int j = 0; j < pLights->Size(); ++j)
		{
			Scene::CLight* pLight = (*pLights)[j];

			if (!IsModelLitByLight(*Rec.pModel, *pLight)) continue;

			//???always add global directional light if it exists?
			//even if its priority is low? or force its high priority?
			// It will always be at index 0 (or at last all dir lights are at the beginning of the array)
			// So we can assume directional light(s?) have priority over other lights
			//???what if shadow-casting light doesn't affect object, but not shadow-casting does?
			//or shadow casting lights gather their objects separately, and here only lighting is actual?
			if (Rec.LightCount == MaxLightsPerObject)
			{
				float NewLightPriority = CalcLightPriority(*Rec.pModel, *pLight);
				DWORD MinIdx = MaxLightsPerObject;
				float MinPriority = FLT_MAX;
				for (DWORD k = 0; k < MaxLightsPerObject; ++k)
				{
					if (Rec.LightPriorities[k] < 0.f)
						Rec.LightPriorities[k] = CalcLightPriority(*Rec.pModel, *Rec.Lights[k]);

					if (Rec.LightPriorities[k] < MinPriority)
					{
						MinPriority = Rec.LightPriorities[k];
						MinIdx = k;
					}
				}

				if (NewLightPriority > MinPriority)
				{
					for (DWORD k = MinIdx; k < MaxLightsPerObject - 1; ++k)
						Rec.Lights[k] = Rec.Lights[k + 1];
					Rec.Lights[MaxLightsPerObject - 1] = pLight;
					Rec.LightPriorities[MaxLightsPerObject - 1] = NewLightPriority;
				}
			}
			else
			{
				Rec.Lights[Rec.LightCount] = pLight;
				Rec.LightPriorities[Rec.LightCount] = -1.f; // For lazy calculation above
				++Rec.LightCount;
			}
		}
		n_assert_dbg(Rec.LightCount <= MaxLightsPerObject);
		//LIGHTS-

		Rec.hTech = Rec.pModel->Material->GetShader()->GetTechByFeatures(Rec.FeatFlags);
		n_assert(Rec.hTech);

		//???need distance to BBox? May be critical for Alpha geometry!
		if (DistanceSorting != Sort_None)
			Rec.SqDistanceToCamera = vector3::SqDistance(Rec.pModel->GetNode()->GetWorldPosition(), EyePos);
	}

	if (Models.Size() > 1)
		switch (DistanceSorting)
		{
			case Sort_None:			Models.Sort<CRecCmp_TechMtlGeom>(); break;
			case Sort_FrontToBack:	Models.Sort<CRecCmp_DistFtB>(); break;
			case Sort_BackToFront:	Models.Sort<CRecCmp_DistBtF>(); break;
		}

	CShader::HTech	hTech = NULL;
	CMaterial*		pMaterial = NULL;
	bool			InstancingIsActive = false;
	CShader::HVar	hWorld;
	CShader::HVar	hWVP;

	if (Shader.isvalid())
	{
		for (int i = 0; i < ShaderVars.Size(); ++i)
			ShaderVars.ValueAtIndex(i).Apply(*Shader.get_unsafe());
		n_assert(Shader->Begin(true) == 1); //!!!PERF: saves state!
		Shader->BeginPass(0);
	}

	for (int i = 0; i < Models.Size(); /*NB: i is incremented inside*/)
	{
		CModelRecord& Rec = Models[i];

		bool NeedToApplyStaticVars = false;

		// Remember current material. If tech will change, we will end old tech through it.
		CMaterial* pPrevMaterial = pMaterial;
		if (pMaterial != Rec.pModel->Material)
		{
			pMaterial = Rec.pModel->Material.get_unsafe();
			NeedToApplyStaticVars = true;
		}
		n_assert_dbg(pMaterial);

		// Manage instancing

		// All objects with the same material must use the same set of per-object variables.
		// Since we sort by geometry, there can't be deformed and static geometry in the same set.
		// Per-object vars are sorted by ID in dictionary, so order is maintained implicitly.
		// Given that, we can compare per-object vars one by one if we want to find instances.

		// NB: Now I use fast instancing approach, when instancing is allowed only for objects
		// without per-object vars. Maybe per-variable value comparison will lead to performance
		// hit because of additional work that brings no significant effect. Needs profiling.

		CMesh* pMesh = Rec.pModel->Mesh.get_unsafe();
		n_assert_dbg(pMesh);
		DWORD GroupIdx = Rec.pModel->MeshGroupIndex;

		//???search only when base tech changes?
		CShader::HTech hInstancedTech =
			pMaterial->GetShader()->GetTechByFeatures(Rec.FeatFlags | RenderSrv->GetFeatureFlagInstanced());

		int j = i + 1;

		if (hInstancedTech && InstanceBuffer.isvalid() && !Rec.pModel->ShaderVars.Size())
			while (	j < Models.Size() &&
					Models[j].pModel->MeshGroupIndex == GroupIdx &&
					Models[j].pModel->Mesh.get_unsafe() == pMesh &&
					Models[j].pModel->Material.get_unsafe() == pMaterial &&
					Models[j].hTech == Rec.hTech)
				++j;

		DWORD InstanceCount = j - i;

		// Select new shader technique taking instancing into account

		CShader::HTech hNewTech = (InstanceCount > 1) ? hInstancedTech : Rec.hTech;
		bool TechChanged = (hTech != hNewTech);

		// Set new tech, end previous tech if it was beginned

		if (TechChanged)
		{
			if (pPrevMaterial)
			{
				pPrevMaterial->GetShader()->EndPass();
				pPrevMaterial->GetShader()->End();
			}

			hTech = hNewTech;
			n_assert_dbg(pMaterial->GetShader()->SetTech(hTech));
			NeedToApplyStaticVars = true;
		}

		// Apply shader variables

		bool ShaderVarsChanged = false;

		if (NeedToApplyStaticVars)
		{
			//!!!Apply() as method to CShaderVarMap! Mb even store shader ref in it.
			//???check IsVarUsed?
			for (int VarIdx = 0; VarIdx < pMaterial->GetStaticVars().Size(); ++VarIdx)
				if (pMaterial->GetStaticVars().ValueAtIndex(VarIdx).IsBound())
					n_assert(pMaterial->GetStaticVars().ValueAtIndex(VarIdx).Apply(*pMaterial->GetShader()));
			ShaderVarsChanged = (pMaterial->GetStaticVars().Size() > 0);
		}

		//!!!Apply() as method to CShaderVarMap! Mb even store shader ref in it.
		//???check IsVarUsed?
		for (int VarIdx = 0; VarIdx < Rec.pModel->ShaderVars.Size(); ++VarIdx)
			if (Rec.pModel->ShaderVars.ValueAtIndex(VarIdx).IsBound())
				n_assert(Rec.pModel->ShaderVars.ValueAtIndex(VarIdx).Apply(*pMaterial->GetShader()));
		ShaderVarsChanged = ShaderVarsChanged || (Rec.pModel->ShaderVars.Size() > 0);

		// Setup or disable instancing, setup World transform

		if (InstanceCount > 1)
		{
			//!!!if > MaxInstanceCount, split!
			n_assert(InstanceCount <= MaxInstanceCount);

			//!!!can supply WVP when more appropriate!
			matrix44* pInstData = (matrix44*)InstanceBuffer->Map(MapWriteDiscard);
			n_assert_dbg(pInstData);
			for (int InstIdx = i; InstIdx < j; ++InstIdx)
				*pInstData++ = Models[InstIdx].pModel->GetNode()->GetWorldMatrix();
			InstanceBuffer->Unmap();

			//!!!make sure that there are no redundant concatenations inside one frame or at least in succession!
			nArray<CVertexComponent> InstComponents = pMesh->GetVertexBuffer()->GetVertexLayout()->GetComponents();
			InstComponents.AppendArray(InstanceBuffer->GetVertexLayout()->GetComponents());
			RenderSrv->SetVertexLayout(RenderSrv->GetVertexLayout(InstComponents));
			RenderSrv->SetInstanceBuffer(1, InstanceBuffer, InstanceCount); //???don't hardcode instance stream index?
			InstancingIsActive = true;
		}
		else
		{
			if (TechChanged)
			{
				// PPL shaders use World tfm on position, so we can then mul it on VP, which is
				// set once per frame. But some rare non-PPL shaders use WVP matrix, and for this
				// case we set WVP and not World.
				hWorld = pMaterial->GetShader()->GetVarHandleByName(CStrID("World"));
				if (!pMaterial->GetShader()->IsVarUsed(hWorld)) hWorld = NULL;
				hWVP = pMaterial->GetShader()->GetVarHandleByName(CStrID("WorldViewProjection"));
				if (!pMaterial->GetShader()->IsVarUsed(hWVP)) hWVP = NULL;
			}

			const matrix44& World = Rec.pModel->GetNode()->GetWorldMatrix();

			if (hWorld) pMaterial->GetShader()->SetMatrix(hWorld, World);
			if (hWVP) pMaterial->GetShader()->SetMatrix(hWVP, World * RenderSrv->GetViewProjection());

			ShaderVarsChanged = ShaderVarsChanged || hWorld || hWVP;

			RenderSrv->SetVertexLayout(pMesh->GetVertexBuffer()->GetVertexLayout());
			if (InstancingIsActive)
			{
				RenderSrv->SetInstanceBuffer(1, NULL, 0); //???don't hardcode instance stream index?
				InstancingIsActive = false;
			}
		}

		i = j;

		// Bring shader up to date and ready for rendering

		if (TechChanged)
		{
			DWORD PassCount = pMaterial->GetShader()->Begin(false); //!!!savestate!
			n_assert_dbg(PassCount == 1); //???allow multipass shaders?
			pMaterial->GetShader()->BeginPass(0);
		}
		else if (ShaderVarsChanged) pMaterial->GetShader()->CommitChanges();

		// Render geometry

		RenderSrv->SetVertexBuffer(0, pMesh->GetVertexBuffer());
		RenderSrv->SetIndexBuffer(pMesh->GetIndexBuffer());
		RenderSrv->SetPrimitiveGroup(pMesh->GetGroup(GroupIdx));
		RenderSrv->Draw();
	}

	if (pMaterial)
	{
		pMaterial->GetShader()->EndPass();
		pMaterial->GetShader()->End();
	}

	if (Shader.isvalid())
	{
		Shader->EndPass();
		Shader->End();
	}

	Models.Clear();
}
//---------------------------------------------------------------------

}