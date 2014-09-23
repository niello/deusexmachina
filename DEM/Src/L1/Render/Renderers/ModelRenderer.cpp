#include "ModelRenderer.h"

#include <Render/SPS.h>
#include <Render/RenderServer.h>
#include <Data/Params.h>
#include <Math/Sphere.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CModelRenderer, 'MLRN', Render::IRenderer);

bool CModelRenderer::Init(const Data::CParams& Desc)
{
	CStrID ShaderID = Desc.Get(CStrID("Shader"), CStrID::Empty);
	if (ShaderID.IsValid())
	{
		Shader = RenderSrv->ShaderMgr.GetTypedResource(ShaderID);
		if (!Shader->IsLoaded()) FAIL;
	}

	//!!!DUPLICATE CODE!+
	ShaderVars.BeginAdd();

	//???to shadervarmap method?
	Data::CParam* pPrm;
	if (Desc.Get(pPrm, CStrID("ShaderVars")))
	{
		Data::CParams& Vars = *pPrm->GetValue<Data::PParams>();
		for (int i = 0; i < Vars.GetCount(); ++i)
		{
			Data::CParam& PrmVar = Vars.Get(i);
			CShaderVar& Var = ShaderVars.Add(PrmVar.GetName());
			Var.SetName(PrmVar.GetName());
			Var.Value = PrmVar.GetRawValue();
		}
	}

	//???to shadervarmap method?
	if (Desc.Get(pPrm, CStrID("Textures"))) //!!!can use string vars in main block instead!
	{
		Data::CParams& Vars = *pPrm->GetValue<Data::PParams>();
		for (int i = 0; i < Vars.GetCount(); ++i)
		{
			Data::CParam& PrmVar = Vars.Get(i);
			CShaderVar& Var = ShaderVars.Add(PrmVar.GetName());
			Var.SetName(PrmVar.GetName());
			Var.Value = RenderSrv->TextureMgr.GetOrCreateTypedResource(CStrID(PrmVar.GetValue<CString>().CStr()));
		}
	}

	ShaderVars.EndAdd();
	//!!!DUPLICATE CODE!-

	BatchType = Desc.Get<CStrID>(CStrID("BatchType"));
	n_assert(BatchType.IsValid());

	FeatFlags = RenderSrv->ShaderFeatures.GetMask(Desc.Get<CString>(CStrID("FeatFlags"), NULL));

	CString SortType;
	if (Desc.Get<CString>(SortType, CStrID("Sort")))
	{
		SortType.TrimInplace();
		SortType.ToLower();
		if (SortType == "fronttoback" || SortType == "ftb")
			DistanceSorting = Sort_FrontToBack;
		else if (SortType == "backtofront" || SortType == "btf")
			DistanceSorting = Sort_BackToFront;
		else DistanceSorting = Sort_None;
	}

	EnableLighting = Desc.Get<bool>(CStrID("EnableLighting"), false);

	if (EnableLighting)
	{
		SharedShader = RenderSrv->ShaderMgr.GetTypedResource(CStrID("Shared"));
		n_assert(SharedShader.IsValid());

		hLightType = SharedShader->GetVarHandleByName(CStrID("LightType"));
		hLightPos = SharedShader->GetVarHandleByName(CStrID("LightPos"));
		hLightDir = SharedShader->GetVarHandleByName(CStrID("LightDir"));
		hLightColor = SharedShader->GetVarHandleByName(CStrID("LightColor"));
		hLightParams = SharedShader->GetVarHandleByName(CStrID("LightParams"));

		for (DWORD i = 0; i < MaxLightsPerObject; ++i)
		{
			CString Mask;
			Mask.Format("L%d", i + 1);
			LightFeatFlags[i] = RenderSrv->ShaderFeatures.GetMask(Mask);
		}
	}

	//???add InitialInstanceCount + AllowGrowInstanceBuffer or MaxInstanceCount or both?
	MaxInstanceCount = Desc.Get<int>(CStrID("MaxInstanceCount"), 0);
	if (MaxInstanceCount) InstanceBuffer = n_new(CVertexBuffer);
	//!!!InstanceBuffer is created lazy in Render() not to duplicate code!

	OK;
}
//---------------------------------------------------------------------

void CModelRenderer::AddRenderObjects(const CArray<CRenderObject*>& Objects)
{
	for (int i = 0; i < Objects.GetCount(); ++i)
	{
		//???use buckets instead?
		if (!Objects[i]->IsA(CModel::RTTI)) continue;
		CModel* pModel = (CModel*)Objects[i];

		n_assert_dbg(pModel->BatchType.IsValid());
		if (pModel->BatchType != BatchType) continue;

		CModelRecord* pRec = Models.Reserve(1);
		pRec->pModel = pModel;
		pRec->FeatFlags = pModel->FeatureFlags | pModel->Material->GetFeatureFlags() | FeatFlags;
		pRec->LightCount = 0;
	}
}
//---------------------------------------------------------------------

//!!!skip black lights or lights with 0 intensity!
void CModelRenderer::AddLights(const CArray<CLight*>& Lights)
{
	pLights = EnableLighting ? &Lights : NULL;
}
//---------------------------------------------------------------------

//!!!pass const refs! all calculations must be done earlier in scene or externally!
bool CModelRenderer::IsModelLitByLight(CModel& Model, CLight& Light)
{
	if (Light.Type == CLight::Directional) OK;

	// Check whether this light potentially touches the model
	if (!Model.pSPSRecord->pSPSNode->SharesSpaceWith(*Light.pSPSRecord->pSPSNode)) FAIL;

	//!!!GetGlobalAABB must return cached box, scene updates it!
	CAABB ModelBox;
	Model.GetGlobalAABB(ModelBox);

	// Now test spotlight too. In fact spotlight sphere is much less.
	sphere LightSphere(Light.GetNode()->GetWorldPosition(), Light.GetRange());
	if (LightSphere.GetClipStatus(ModelBox) == Outside) FAIL;

	if (Light.Type == CLight::Spot)
	{
		//!!!precalculate once!
		matrix44 LightFrustum;
		Light.CalcFrustum(LightFrustum);
		if (ModelBox.GetClipStatus(LightFrustum) == Outside) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

//!!!pass const refs! all calculations must be done earlier in scene or externally!
// NB: always returns positive number.
float CModelRenderer::CalcLightPriority(CModel& Model, CLight& Light)
{
	// I probably should test for a closest point on or inside bbox, but for now only position is used.
	// Light shining from the center of the object doesn't affect it or does it wrong.

	float SqIntensity = Light.Intensity * Light.Intensity;
	if (Light.Type == CLight::Directional) return SqIntensity;

	float SqDistance = vector3::SqDistance(Model.GetNode()->GetWorldPosition(), Light.GetNode()->GetWorldPosition());
	float Attenuation = (1.f - SqDistance * (Light.GetInvRange() * Light.GetInvRange()));

	if (Light.Type == CLight::Spot && SqDistance != 0.f)
	{
		vector3 ModelLight = Model.GetNode()->GetWorldPosition() - Light.GetNode()->GetWorldPosition();
		ModelLight /= n_sqrt(SqDistance);
		float CosAlpha = ModelLight.Dot(-Light.GetDirection());
		float Falloff = (CosAlpha - Light.GetCosHalfPhi()) / (Light.GetCosHalfTheta() - Light.GetCosHalfPhi());
		return SqIntensity * Attenuation * Clamp(Falloff, 0.f, 1.f);
	}

	return SqIntensity * Attenuation;
}
//---------------------------------------------------------------------

void CModelRenderer::Render()
{
	if (!Models.GetCount()) return;

	if (MaxInstanceCount && !InstanceBuffer->IsValid())
	{
		CArray<CVertexComponent> InstCmps(4, 0);
		for (int i = 0; i < 4; ++i)
		{
			CVertexComponent& Cmp = InstCmps.At(i);
			Cmp.Format = CVertexComponent::Float4;
			Cmp.Semantic = CVertexComponent::TexCoord;
			Cmp.Index = i + 4; // TEXCOORD 4, 5, 6, 7 are used
			Cmp.Stream = 1;
		}
		n_assert(InstanceBuffer->Create(RenderSrv->GetVertexLayout(InstCmps), MaxInstanceCount, Usage_Dynamic, CPU_Write));
	}

	// Prepare to sorting

	vector3 EyePos = RenderSrv->GetCameraPosition();
	for (int i = 0; i < Models.GetCount(); ++i)
	{
		CModelRecord& Rec = Models[i];

		// Gather lights that light current model

		if (EnableLighting)
		{
			n_assert_dbg(pLights);
			for (int j = 0; j < pLights->GetCount(); ++j)
			{
				CLight* pLight = (*pLights)[j];

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
						//!!!since now I reset the whole light array, order doesn't matter!
						//for (DWORD k = MinIdx; k < MaxLightsPerObject - 1; ++k)
						//	Rec.Lights[k] = Rec.Lights[k + 1];
						//Rec.Lights[MaxLightsPerObject - 1] = pLight;
						//Rec.LightPriorities[MaxLightsPerObject - 1] = NewLightPriority;
						Rec.Lights[MinIdx] = pLight;
						Rec.LightPriorities[MinIdx] = NewLightPriority;
					}
				}
				else
				{
					Rec.Lights[Rec.LightCount] = pLight;
					Rec.LightPriorities[Rec.LightCount] = -1.f; // For lazy calculation above
					++Rec.LightCount;
				}
			}

			if (Rec.LightCount > 0)
			{
				n_assert_dbg(Rec.LightCount <= MaxLightsPerObject);
				Rec.FeatFlags |= LightFeatFlags[Rec.LightCount - 1];
			}
		}

		Rec.hTech = Rec.pModel->Material->GetShader()->GetTechByFeatures(Rec.FeatFlags);
		n_assert(Rec.hTech);

		//???need distance to BBox? May be critical for Alpha geometry!
		if (DistanceSorting != Sort_None)
			Rec.SqDistanceToCamera = vector3::SqDistance(Rec.pModel->GetNode()->GetWorldPosition(), EyePos);
	}

	// Sort models

	if (Models.GetCount() > 1)
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

	// Render models one by one

	if (Shader.IsValid())
	{
		for (int i = 0; i < ShaderVars.GetCount(); ++i)
			ShaderVars.ValueAt(i).Apply(*Shader.GetUnsafe());
		n_assert(Shader->Begin(true) == 1); //!!!PERF: saves state!
		Shader->BeginPass(0);
	}

	for (int i = 0; i < Models.GetCount(); /*NB: i is incremented inside*/)
	{
		CModelRecord& Rec = Models[i];

		bool NeedToApplyStaticVars = false;

		// Remember current material. If tech will change, we will end old tech through it.
		CMaterial* pPrevMaterial = pMaterial;
		if (pMaterial != Rec.pModel->Material)
		{
			pMaterial = Rec.pModel->Material.GetUnsafe();
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

		CMesh* pMesh = Rec.pModel->Mesh.GetUnsafe();
		n_assert_dbg(pMesh);
		DWORD GroupIdx = Rec.pModel->MeshGroupIndex;

		//???search only when base tech changes?
		CShader::HTech hInstancedTech =
			pMaterial->GetShader()->GetTechByFeatures(Rec.FeatFlags | RenderSrv->GetFeatureFlagInstanced());

		int j = i + 1;

		if (hInstancedTech && InstanceBuffer.IsValid() && !Rec.pModel->ShaderVars.GetCount())
			while (	j < Models.GetCount() &&
					Models[j].pModel->MeshGroupIndex == GroupIdx &&
					Models[j].pModel->Mesh.GetUnsafe() == pMesh &&
					Models[j].pModel->Material.GetUnsafe() == pMaterial &&
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
			n_verify_dbg(pMaterial->GetShader()->SetTech(hTech));
			NeedToApplyStaticVars = true;
		}

		// Apply shader variables

		bool ShaderVarsChanged = false;

		if (NeedToApplyStaticVars)
		{
			//!!!Apply() as method to CShaderVarMap! Mb even store shader ref in it.
			//???check IsVarUsed?
			for (int VarIdx = 0; VarIdx < pMaterial->GetStaticVars().GetCount(); ++VarIdx)
				if (pMaterial->GetStaticVars().ValueAt(VarIdx).IsBound())
					n_assert(pMaterial->GetStaticVars().ValueAt(VarIdx).Apply(*pMaterial->GetShader()));
			ShaderVarsChanged = (pMaterial->GetStaticVars().GetCount() > 0);
		}

		//!!!Apply() as method to CShaderVarMap! Mb even store shader ref in it.
		//???check IsVarUsed?
		for (int VarIdx = 0; VarIdx < Rec.pModel->ShaderVars.GetCount(); ++VarIdx)
			if (Rec.pModel->ShaderVars.ValueAt(VarIdx).IsBound())
				n_assert(Rec.pModel->ShaderVars.ValueAt(VarIdx).Apply(*pMaterial->GetShader()));
		ShaderVarsChanged = ShaderVarsChanged || (Rec.pModel->ShaderVars.GetCount() > 0);

		//!!!Need redundancy check, may be even "does this light now set at ANY index"! mb store index in light rec
		//!!!light param arrays are local because now I don't know how to set only part of shader var array,
		// so, I reset whole array each time!
		if (Rec.LightCount)
		{
			//!!!can skip if all lights are the same as at the previous set (no matter in what order)!
			int LightType[MaxLightsPerObject];
			vector4 LightPos[MaxLightsPerObject];
			vector4 LightDir[MaxLightsPerObject];
			vector4 LightColor[MaxLightsPerObject];
			vector4 LightParams[MaxLightsPerObject];
			bool HasPointOrSpotLights = false;
			bool HasDirOrSpotLights = false;
			for (DWORD LightIdx = 0; LightIdx < Rec.LightCount; ++LightIdx)
			{
				CLight& Light = *Rec.Lights[LightIdx];
				LightType[LightIdx] = (int)Light.Type;
				LightColor[LightIdx] = Light.Color * Light.Intensity;
				if (Light.Type == CLight::Directional)
				{
					HasDirOrSpotLights = true;
					LightDir[LightIdx] = Light.GetReverseDirection();
				}
				else
				{
					HasPointOrSpotLights = true;
					LightPos[LightIdx] = Light.GetPosition();
					LightParams[LightIdx].x = Light.GetRange(); //!!!set inverse range!
					if (Light.Type == CLight::Spot)
					{
						HasDirOrSpotLights = true;
						LightDir[LightIdx] = Light.GetDirection();
						//???precalculate 1/(cosT/2 - cosP/2) here?
						LightParams[LightIdx].y = Light.GetCosHalfTheta();
						LightParams[LightIdx].z = Light.GetCosHalfPhi();
					}
				}
			}

			if (hLightType) SharedShader->SetIntArray(hLightType, LightType, Rec.LightCount);
			if (hLightColor) SharedShader->SetFloat4Array(hLightColor, LightColor, Rec.LightCount);
			if (HasDirOrSpotLights && hLightDir) SharedShader->SetFloat4Array(hLightDir, LightDir, Rec.LightCount);
			if (HasPointOrSpotLights)
			{
				if (hLightPos) SharedShader->SetFloat4Array(hLightPos, LightPos, Rec.LightCount);
				if (hLightParams) SharedShader->SetFloat4Array(hLightParams, LightParams, Rec.LightCount);
			}
		}

		// Setup or disable instancing, setup World transform

		if (InstanceCount > 1)
		{
			//!!!if > MaxInstanceCount, split!
			n_assert(InstanceCount <= MaxInstanceCount);

			//!!!can supply WVP when more appropriate!
			matrix44* pInstData = (matrix44*)InstanceBuffer->Map(Map_WriteDiscard);
			n_assert_dbg(pInstData);
			for (int InstIdx = i; InstIdx < j; ++InstIdx)
				*pInstData++ = Models[InstIdx].pModel->GetNode()->GetWorldMatrix();
			InstanceBuffer->Unmap();

			//!!!make sure that there are no redundant concatenations inside one frame or at least in succession!
			CArray<CVertexComponent> InstComponents = pMesh->GetVertexBuffer()->GetVertexLayout()->GetComponents();
			InstComponents.AddArray(InstanceBuffer->GetVertexLayout()->GetComponents());
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

	if (InstancingIsActive)
	{
		RenderSrv->SetInstanceBuffer(1, NULL, 0); //???don't hardcode instance stream index?
		InstancingIsActive = false;
	}

	if (pMaterial)
	{
		pMaterial->GetShader()->EndPass();
		pMaterial->GetShader()->End();
	}

	if (Shader.IsValid())
	{
		Shader->EndPass();
		Shader->End();
	}

	Models.Clear();
	pLights = NULL; //!!!clear!
}
//---------------------------------------------------------------------

}