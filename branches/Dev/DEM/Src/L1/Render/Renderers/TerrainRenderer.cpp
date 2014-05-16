#include "TerrainRenderer.h"

#include <Scene/Light.h>
#include <Render/RenderServer.h>
#include <Data/Params.h>
#include <Math/sphere.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CTerrainRenderer, 'TRRE', Render::IRenderer);

bool CTerrainRenderer::Init(const Data::CParams& Desc)
{
	CStrID ShaderID = Desc.Get(CStrID("Shader"), CStrID::Empty);
	if (ShaderID.IsValid())
	{
		Shader = RenderSrv->ShaderMgr.GetTypedResource(ShaderID);
		if (!Shader.IsValid() || !Shader->IsLoaded()) FAIL;
	}
	else FAIL;

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

	FeatFlags = RenderSrv->ShaderFeatures.GetMask(Desc.Get<CString>(CStrID("FeatFlags"), NULL));

	if (!RenderSrv->CheckCaps(Caps_VSTexFiltering_Linear))
	{
		Sys::Error("Fix feat flags for this case to use fallback tech with manual 4-sample filtering");
	}

	hHeightMap = Shader->GetVarHandleByName(CStrID("HeightMap"));
	hWorldToHM = Shader->GetVarHandleByName(CStrID("WorldToHM"));
	hTerrainYInvSplat = Shader->GetVarHandleByName(CStrID("TerrainYInvSplat"));
	hGridConsts = Shader->GetVarHandleByName(CStrID("GridConsts"));
	hHMTexInfo = Shader->GetVarHandleByName(CStrID("HMTexInfo"));

	EnableLighting = Desc.Get<bool>(CStrID("EnableLighting"), false);

	if (EnableLighting)
	{
		SharedShader = RenderSrv->ShaderMgr.GetTypedResource(CStrID("Shared"));
		n_assert(SharedShader.IsValid());

		hLightType = SharedShader->GetVarHandleByName(CStrID("LightType"));
		hLightDir = SharedShader->GetVarHandleByName(CStrID("LightDir"));
		hLightColor = SharedShader->GetVarHandleByName(CStrID("LightColor"));

		for (DWORD i = 0; i < MaxLightsPerObject; ++i)
		{
			CString Mask;
			Mask.Format("L%d", i + 1);
			LightFeatFlags[i] = RenderSrv->ShaderFeatures.GetMask(Mask);
		}
	}

	FeatFlagDefault = RenderSrv->ShaderFeatures.GetMask("Default");
	if (!FeatFlags && !EnableLighting) FeatFlags = FeatFlagDefault;

	CArray<CVertexComponent> InstCmps(2, 0);

	// ScaleOffset
	CVertexComponent* pCmp = InstCmps.Reserve(2);
	pCmp->Format = CVertexComponent::Float4;
	pCmp->Semantic = CVertexComponent::TexCoord;
	pCmp->Index = 0;
	pCmp->Stream = 1;

	// MorphConsts
	++pCmp;
	pCmp->Format = CVertexComponent::Float2;
	pCmp->Semantic = CVertexComponent::TexCoord;
	pCmp->Index = 1;
	pCmp->Stream = 1;

	// NB: Depends on CTerrain mesh vertex format
	CArray<CVertexComponent> PatchVC;
	CVertexComponent& Cmp = *PatchVC.Reserve(1);
	Cmp.Format = CVertexComponent::Float2;
	Cmp.Semantic = CVertexComponent::Position;
	Cmp.Index = 0;
	Cmp.Stream = 0;

	PatchVC.AddArray(InstCmps);
	FinalVertexLayout = RenderSrv->GetVertexLayout(PatchVC);

	//???add InitialInstanceCount + AllowGrowInstanceBuffer or MaxInstanceCount or both?
	//!!!ALLOW GROW OF InstanceBuffer!
	MaxInstanceCount = Desc.Get<int>(CStrID("MaxInstanceCount"), 256);
	n_assert(MaxInstanceCount);
	InstanceBuffer = n_new(CVertexBuffer);
	InstanceVertexLayout = RenderSrv->GetVertexLayout(InstCmps);
	n_assert(InstanceBuffer->Create(InstanceVertexLayout, MaxInstanceCount, Usage_Dynamic, CPU_Write));

	OK;
}
//---------------------------------------------------------------------

void CTerrainRenderer::AddRenderObjects(const CArray<Scene::CRenderObject*>& Objects)
{
	for (int i = 0; i < Objects.GetCount(); ++i)
	{
		//???use buckets instead?
		if (!Objects[i]->IsA<Scene::CTerrain>()) continue;
		TerrainObjects.Add((Scene::CTerrain*)Objects[i]);
	}
}
//---------------------------------------------------------------------

void CTerrainRenderer::AddLights(const CArray<Scene::CLight*>& Lights)
{
	pLights = EnableLighting ? &Lights : NULL;
}
//---------------------------------------------------------------------

// Need to cache calculations between Depth & Color passes, but now there are 2 different rendrers. Cache in CTerrain?
// Can calculate distance to camera when sorting by distance is needed
// Maybe sorting by LOD is enough
// Also can return the highest and the lowest LODs
CTerrainRenderer::ENodeStatus CTerrainRenderer::ProcessNode(Scene::CTerrain& Terrain, DWORD X, DWORD Z,
															DWORD LOD, float LODRange, CPatchInstance* pInstances,
															DWORD& PatchCount, DWORD& QPatchCount, EClipStatus Clip)
{
	short MinY, MaxY;
	Terrain.GetMinMaxHeight(X, Z, LOD, MinY, MaxY);

	// Node has no data, skip it completely
	if (MaxY < MinY) return Node_Invisible;

	DWORD NodeSize = Terrain.GetPatchSize() << LOD;
	CAABB TerrainAABB;
	Terrain.GetGlobalAABB(TerrainAABB); //???get once outside and pass as param?
	float ScaleX = NodeSize * (TerrainAABB.vmax.x - TerrainAABB.vmin.x) / (float)(Terrain.GetHeightMapWidth() - 1);
	float ScaleZ = NodeSize * (TerrainAABB.vmax.z - TerrainAABB.vmin.z) / (float)(Terrain.GetHeightMapHeight() - 1);

	CAABB AABB;
	AABB.vmin.x = TerrainAABB.vmin.x + X * ScaleX;
	AABB.vmax.x = TerrainAABB.vmin.x + (X + 1) * ScaleX;
	AABB.vmin.y = MinY * Terrain.GetVerticalScale();
	AABB.vmax.y = MaxY * Terrain.GetVerticalScale();
	AABB.vmin.z = TerrainAABB.vmin.z + Z * ScaleZ;
	AABB.vmax.z = TerrainAABB.vmin.z + (Z + 1) * ScaleZ;

	if (Clip == Clipped)
	{
		Clip = AABB.GetClipStatus(RenderSrv->GetViewProjection());
		if (Clip == Outside) return Node_Invisible;
	}

	//!!!don't create sphere object for test!
	sphere LODSphere(RenderSrv->GetCameraPosition(), LODRange); //!!!Always must check the Main camera!
	if (LODSphere.GetClipStatus(AABB) == Outside) return Node_NotInLOD;

	// Flags identifying what children we need to add
	bool TL = true, TR = true, BL = true, BR = true, AddWhole, IsVisible;

	if (LOD > 0)
	{
		// Hack, see original CDLOD code. LOD 0 range is 0.9 of what is expected.
		float NextLODRange = LODRange * ((LOD == 1) ? 0.45f : 0.5f);

		IsVisible = false;

		//!!!don't create sphere object for test!
		sphere LODSphere(RenderSrv->GetCameraPosition(), NextLODRange); //!!!Always must check the Main camera!
		EClipStatus NextClip = LODSphere.GetClipStatus(AABB);
		if (NextClip != Outside)
		{
			DWORD XNext = X << 1, ZNext = Z << 1;

			ENodeStatus Status = ProcessNode(Terrain, XNext, ZNext, LOD - 1, NextLODRange, pInstances, PatchCount, QPatchCount, NextClip);
			TL = (Status == Node_NotInLOD);
			IsVisible |= (Status != Node_Invisible);

			TR = Terrain.HasNode(XNext + 1, ZNext, LOD - 1);
			if (TR)
			{
				Status = ProcessNode(Terrain, XNext + 1, ZNext, LOD - 1, NextLODRange, pInstances, PatchCount, QPatchCount, NextClip);
				TR = (Status == Node_NotInLOD);
				IsVisible |= (Status != Node_Invisible);
			}

			BL = Terrain.HasNode(XNext, ZNext + 1, LOD - 1);
			if (BL)
			{
				Status = ProcessNode(Terrain, XNext, ZNext + 1, LOD - 1, NextLODRange, pInstances, PatchCount, QPatchCount, NextClip);
				BL = (Status == Node_NotInLOD);
				IsVisible |= (Status != Node_Invisible);
			}

			BR = Terrain.HasNode(XNext + 1, ZNext + 1, LOD - 1);
			if (BR)
			{
				Status = ProcessNode(Terrain, XNext + 1, ZNext + 1, LOD - 1, NextLODRange, pInstances, PatchCount, QPatchCount, NextClip);
				BR = (Status == Node_NotInLOD);
				IsVisible |= (Status != Node_Invisible);
			}
		}

		AddWhole = TL && TR && BL && BR;
	}
	else
	{
		AddWhole = true;
		IsVisible = true;
	}

	if (!TL && !TR && !BL && !BR) return IsVisible ? Node_Processed : Node_Invisible;

	if (AddWhole)
	{
		n_assert(PatchCount + QPatchCount < MaxInstanceCount);
		CPatchInstance& Patch = pInstances[PatchCount];
		Patch.ScaleOffset.x = AABB.vmax.x - AABB.vmin.x;
		Patch.ScaleOffset.y = AABB.vmax.z - AABB.vmin.z;
		Patch.ScaleOffset.z = AABB.vmin.x;
		Patch.ScaleOffset.w = AABB.vmin.z;
		Patch.MorphConsts[0] = MorphConsts[LOD].Const1;
		Patch.MorphConsts[1] = MorphConsts[LOD].Const2;
		++PatchCount;
	}
	else
	{
		// Add quarterpatches

		float HalfScaleX = (AABB.vmax.x - AABB.vmin.x) * 0.5f;
		float HalfScaleZ = (AABB.vmax.z - AABB.vmin.z) * 0.5f;

		if (TL)
		{
			n_assert(PatchCount + QPatchCount < MaxInstanceCount);
			CPatchInstance& Patch = pInstances[MaxInstanceCount - QPatchCount - 1];
			Patch.ScaleOffset.x = HalfScaleX;
			Patch.ScaleOffset.y = HalfScaleZ;
			Patch.ScaleOffset.z = AABB.vmin.x;
			Patch.ScaleOffset.w = AABB.vmin.z;
			Patch.MorphConsts[0] = MorphConsts[LOD].Const1;
			Patch.MorphConsts[1] = MorphConsts[LOD].Const2;
			++QPatchCount;
		}

		if (TR)
		{
			n_assert(PatchCount + QPatchCount < MaxInstanceCount);
			CPatchInstance& Patch = pInstances[MaxInstanceCount - QPatchCount - 1];
			Patch.ScaleOffset.x = HalfScaleX;
			Patch.ScaleOffset.y = HalfScaleZ;
			Patch.ScaleOffset.z = AABB.vmin.x + HalfScaleX;
			Patch.ScaleOffset.w = AABB.vmin.z;
			Patch.MorphConsts[0] = MorphConsts[LOD].Const1;
			Patch.MorphConsts[1] = MorphConsts[LOD].Const2;
			++QPatchCount;
		}

		if (BL)
		{
			n_assert(PatchCount + QPatchCount < MaxInstanceCount);
			CPatchInstance& Patch = pInstances[MaxInstanceCount - QPatchCount - 1];
			Patch.ScaleOffset.x = HalfScaleX;
			Patch.ScaleOffset.y = HalfScaleZ;
			Patch.ScaleOffset.z = AABB.vmin.x;
			Patch.ScaleOffset.w = AABB.vmin.z + HalfScaleZ;
			Patch.MorphConsts[0] = MorphConsts[LOD].Const1;
			Patch.MorphConsts[1] = MorphConsts[LOD].Const2;
			++QPatchCount;
		}

		if (BR)
		{
			n_assert(PatchCount + QPatchCount < MaxInstanceCount);
			CPatchInstance& Patch = pInstances[MaxInstanceCount - QPatchCount - 1];
			Patch.ScaleOffset.x = HalfScaleX;
			Patch.ScaleOffset.y = HalfScaleZ;
			Patch.ScaleOffset.z = AABB.vmin.x + HalfScaleX;
			Patch.ScaleOffset.w = AABB.vmin.z + HalfScaleZ;
			Patch.MorphConsts[0] = MorphConsts[LOD].Const1;
			Patch.MorphConsts[1] = MorphConsts[LOD].Const2;
			++QPatchCount;
		}
	}

// Morphing artifacts test (from the original CDLOD code)
/*
#ifdef _DEBUG
	if (LOD < Terrain.GetLODCount() - 1)
	{
		//!!!Always must check the Main camera!
		float MaxDistToCameraSq = AABB.MaxDistFromPointSq(RenderSrv->GetCameraPosition());
		float MorphStart = MorphConsts[LOD + 1].Start;
		if (MaxDistToCameraSq > MorphStart * MorphStart)
			Sys::Error("Visibility distance is too small!");
	}
#endif
*/

	return Node_Processed;
}
//---------------------------------------------------------------------

void CTerrainRenderer::Render()
{
	if (!TerrainObjects.GetCount()) return;

	if (!InstanceBuffer->IsValid())
		n_assert(InstanceBuffer->Create(InstanceVertexLayout, MaxInstanceCount, Usage_Dynamic, CPU_Write));

	CShader::HTech hCurrTech = NULL;

	for (int i = 0; i < ShaderVars.GetCount(); ++i)
		ShaderVars.ValueAt(i).Apply(*Shader.GetUnsafe());

	RenderSrv->SetVertexLayout(FinalVertexLayout);

	for (int ObjIdx = 0; ObjIdx < TerrainObjects.GetCount(); ++ObjIdx)
	{
		Scene::CTerrain& Terrain = *TerrainObjects[ObjIdx];

		// Recalculate morphing consts (if cache, need to keep track of both VisibilityRange and LODCount)

		MorphConsts.Clear();

		float PrevPos = 0.f;
		float CurrVisRange = VisibilityRange / (float)(1 << (Terrain.GetLODCount() - 1));
		CMorphInfo* pMorphInfo = MorphConsts.Reserve(Terrain.GetLODCount());
		for (DWORD j = 0; j < Terrain.GetLODCount(); ++j, ++pMorphInfo)
		{
			pMorphInfo->End = CurrVisRange;
			if (j == 0) pMorphInfo->End *= 0.9f;
			pMorphInfo->Start = PrevPos + (pMorphInfo->End - PrevPos) * MorphStartRatio;

			float FixedEnd = n_lerp(pMorphInfo->End, pMorphInfo->Start, 0.01f);
			pMorphInfo->Const2 = 1.0f / (FixedEnd - pMorphInfo->Start);
			pMorphInfo->Const1 = FixedEnd * pMorphInfo->Const2;

			PrevPos = pMorphInfo->Start;
			CurrVisRange *= 2.f;
		}

		// Fill instance data with patches and quarter-patches to render

		DWORD PatchCount = 0, QuarterPatchCount = 0;

		CPatchInstance* pInstances = (CPatchInstance*)InstanceBuffer->Map(Map_WriteDiscard);
		n_assert_dbg(pInstances);

		DWORD TopLOD = Terrain.GetLODCount() - 1;
		for (DWORD Z = 0; Z < Terrain.GetTopPatchCountZ(); ++Z)
			for (DWORD X = 0; X < Terrain.GetTopPatchCountX(); ++X)
				ProcessNode(Terrain, X, Z, TopLOD, VisibilityRange, pInstances, PatchCount, QuarterPatchCount);

		InstanceBuffer->Unmap();

		if (!PatchCount && !QuarterPatchCount) continue;

		// SORTING ======================================================
		//
		//!!!can sort by distance (min dist to camera) before rendering!
		//or can sort by LOD!
		//if so, don't write instance data into the video memory directly,
		//write to tmp storage (with additional fields for sorting), sort, then send to GPU

		// Setup lighting

		DWORD ActualFF = FeatFlags;

		Scene::CLight* pLight = NULL;

		if (EnableLighting)
		{
			n_assert_dbg(pLights);
			for (int j = 0; j < pLights->GetCount(); ++j)
			{
				Scene::CLight& CurrLight = *(*pLights)[j];
				if ((*pLights)[j]->Type == Scene::CLight::Directional || (*pLights)[j]->Intensity == 0.f)
				{
					pLight = (*pLights)[j];
					break; // Now only one directional light is supported by this renderer
				}
			}

			ActualFF |= pLight ? LightFeatFlags[0] : FeatFlagDefault;
		}

		CShader::HTech hNewTech = Shader->GetTechByFeatures(ActualFF);
		if (hCurrTech != hNewTech)
		{
			if (hCurrTech)
			{
				Shader->EndPass();
				Shader->End();
			}
			n_verify_dbg(Shader->SetTech(hNewTech));
			n_assert(Shader->Begin(true) == 1); //!!!PERF: saves state!
			hCurrTech = hNewTech;
		}

		// Setup shader consts

		Shader->SetTexture(hHeightMap, *Terrain.GetHeightMap());

		CAABB TerrainAABB;
		Terrain.GetGlobalAABB(TerrainAABB);
		float WorldToHM[4];
		WorldToHM[0] = 1.f / (TerrainAABB.vmax.x - TerrainAABB.vmin.x);
		WorldToHM[1] = 1.f / (TerrainAABB.vmax.z - TerrainAABB.vmin.z);
		WorldToHM[2] = -TerrainAABB.vmin.x * WorldToHM[0];
		WorldToHM[3] = -TerrainAABB.vmin.z * WorldToHM[1];
		Shader->SetFloatArray(hWorldToHM, WorldToHM, 4);

		float TerrainYInvSplat[4];
		TerrainYInvSplat[0] = 65535.f * Terrain.GetVerticalScale();
		TerrainYInvSplat[1] = -32767.f * Terrain.GetVerticalScale() + Terrain.GetNode()->GetWorldPosition().y;
		TerrainYInvSplat[2] = Terrain.GetInvSplatSizeX();
		TerrainYInvSplat[3] = Terrain.GetInvSplatSizeZ();
		Shader->SetFloatArray(hTerrainYInvSplat, TerrainYInvSplat, 4);

		float HMTexInfo[2];
		HMTexInfo[0] = 1.f / (float)Terrain.GetHeightMapWidth();
		HMTexInfo[1] = 1.f / (float)Terrain.GetHeightMapHeight();
		Shader->SetFloatArray(hHMTexInfo, HMTexInfo, 2);

		for (int VarIdx = 0; VarIdx < Terrain.ShaderVars.GetCount(); ++VarIdx)
		{
			CShaderVar& Var = Terrain.ShaderVars.ValueAt(VarIdx);
			if (!Var.IsBound()) Var.Bind(*Shader);
			if (Var.IsBound()) n_assert(Var.Apply(*Shader));
		}

		if (pLight)
		{
			vector4 LightColor = pLight->Color * pLight->Intensity;
			vector4 LightDir = pLight->GetReverseDirection();
			if (hLightType) SharedShader->SetIntArray(hLightType, (int*)&pLight->Type, 1);
			if (hLightColor) SharedShader->SetFloat4Array(hLightColor, &LightColor, 1);
			if (hLightDir) SharedShader->SetFloat4Array(hLightDir, &LightDir, 1);
		}

		// Render gathered patches

		float GridConsts[2];

		if (PatchCount)
		{
			GridConsts[0] = Terrain.GetPatchSize() * 0.5f;
			GridConsts[1] = 1.f / GridConsts[0];
			Shader->SetFloatArray(hGridConsts, GridConsts, 2);

			if (ObjIdx == 0) Shader->BeginPass(0);
			else Shader->CommitChanges();

			CMesh* pPatch = Terrain.GetPatchMesh();
			n_assert_dbg(pPatch);
			RenderSrv->SetInstanceBuffer(1, InstanceBuffer, PatchCount);
			RenderSrv->SetVertexBuffer(0, pPatch->GetVertexBuffer());
			RenderSrv->SetIndexBuffer(pPatch->GetIndexBuffer());
			RenderSrv->SetPrimitiveGroup(pPatch->GetGroup(0));
			RenderSrv->Draw();
		}

		if (QuarterPatchCount)
		{
			GridConsts[0] = Terrain.GetPatchSize() * 0.25f;
			GridConsts[1] = 1.f / GridConsts[0];
			Shader->SetFloatArray(hGridConsts, GridConsts, 2);

			if (ObjIdx == 0 && !PatchCount) Shader->BeginPass(0);
			else Shader->CommitChanges();

			CMesh* pPatch = Terrain.GetQuarterPatchMesh();
			n_assert_dbg(pPatch);
			RenderSrv->SetInstanceBuffer(1, InstanceBuffer, QuarterPatchCount, MaxInstanceCount - QuarterPatchCount);
			RenderSrv->SetVertexBuffer(0, pPatch->GetVertexBuffer());
			RenderSrv->SetIndexBuffer(pPatch->GetIndexBuffer());
			RenderSrv->SetPrimitiveGroup(pPatch->GetGroup(0));
			RenderSrv->Draw();
		}
	}

	RenderSrv->SetInstanceBuffer(1, NULL, 0);

	if (hCurrTech)
	{
		Shader->EndPass();
		Shader->End();
	}

	TerrainObjects.Clear();
	pLights = NULL;
}
//---------------------------------------------------------------------

}