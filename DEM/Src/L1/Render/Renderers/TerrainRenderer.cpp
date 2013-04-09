#include "TerrainRenderer.h"

#include <Scene/SPS.h>
#include <Render/RenderServer.h>
#include <Data/Params.h>
#include <mathlib/sphere.h>

namespace Render
{
ImplementRTTI(Render::CTerrainRenderer, Render::IRenderer);
ImplementFactory(Render::CTerrainRenderer);

bool CTerrainRenderer::Init(const Data::CParams& Desc)
{
	CStrID ShaderID = Desc.Get(CStrID("Shader"), CStrID::Empty);
	if (ShaderID.IsValid())
	{
		Shader = RenderSrv->ShaderMgr.GetTypedResource(ShaderID);
		if (!Shader->IsLoaded()) FAIL;
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
			Var.Value = RenderSrv->TextureMgr.GetTypedResource(CStrID(PrmVar.GetValue<nString>().Get()));
		}
	}

	ShaderVars.EndAdd();
	//!!!DUPLICATE CODE!-

	FeatFlags = RenderSrv->ShaderFeatureStringToMask(Desc.Get<nString>(CStrID("FeatFlags"), NULL));

	hHeightMap = Shader->GetVarHandleByName(CStrID("HeightMap"));

	EnableLighting = Desc.Get<bool>(CStrID("EnableLighting"), false);

	if (EnableLighting)
	{
		/*
		SharedShader = RenderSrv->ShaderMgr.GetTypedResource(CStrID("Shared"));
		n_assert(SharedShader.isvalid());

		hLightType = SharedShader->GetVarHandleByName(CStrID("LightType"));
		hLightPos = SharedShader->GetVarHandleByName(CStrID("LightPos"));
		hLightDir = SharedShader->GetVarHandleByName(CStrID("LightDir"));
		hLightColor = SharedShader->GetVarHandleByName(CStrID("LightColor"));
		hLightParams = SharedShader->GetVarHandleByName(CStrID("LightParams"));

		for (DWORD i = 0; i < MaxLightsPerObject; ++i)
		{
			nString Mask;
			Mask.Format("L%d", i + 1);
			LightFeatFlags[i] = RenderSrv->ShaderFeatureStringToMask(Mask);
		}
		*/
	}

	nArray<CVertexComponent> PatchVC;
	CVertexComponent& Cmp = *PatchVC.Reserve(1);
	Cmp.Format = CVertexComponent::Float3;
	Cmp.Semantic = CVertexComponent::Position;
	Cmp.Index = 0;
	Cmp.Stream = 0;
	PatchVertexLayout = RenderSrv->GetVertexLayout(PatchVC);

//!!!WRITE APPROPRIATE INSTANCE COMPONENTS!
	nArray<CVertexComponent> InstCmps(4, 4);
	for (int i = 0; i < 4; ++i)
	{
		CVertexComponent& Cmp = InstCmps.At(i);
		Cmp.Format = CVertexComponent::Float4;
		Cmp.Semantic = CVertexComponent::TexCoord;
		Cmp.Index = i + 4; // TEXCOORD 4, 5, 6, 7 are used
		Cmp.Stream = 1;
	}

	PatchVC.AppendArray(InstCmps);
	FinalVertexLayout = RenderSrv->GetVertexLayout(PatchVC);

	//???add InitialInstanceCount + AllowGrowInstanceBuffer or MaxInstanceCount or both?
	//!!!ALLOW GROW OF InstanceBuffer!
	MaxInstanceCount = Desc.Get<int>(CStrID("MaxInstanceCount"), 256);
	n_assert(MaxInstanceCount);
	InstanceBuffer.Create();
	n_assert(InstanceBuffer->Create(RenderSrv->GetVertexLayout(InstCmps), MaxInstanceCount, Usage_Dynamic, CPU_Write));

	OK;
}
//---------------------------------------------------------------------

void CTerrainRenderer::AddRenderObjects(const nArray<Scene::CRenderObject*>& Objects)
{
	for (int i = 0; i < Objects.Size(); ++i)
	{
		//???use buckets instead?
		if (!Objects[i]->IsA<Scene::CTerrain>()) continue;
		TerrainObjects.Append((Scene::CTerrain*)Objects[i]);
	}
}
//---------------------------------------------------------------------

void CTerrainRenderer::AddLights(const nArray<Scene::CLight*>& Lights)
{
	pLights = EnableLighting ? &Lights : NULL;
}
//---------------------------------------------------------------------

void CTerrainRenderer::Render()
{
	if (!TerrainObjects.Size()) return;

	n_verify_dbg(Shader->SetTech(Shader->GetTechByFeatures(FeatFlags)));

	for (int i = 0; i < ShaderVars.Size(); ++i)
		ShaderVars.ValueAtIndex(i).Apply(*Shader.get_unsafe());
	n_assert(Shader->Begin(true) == 1); //!!!PERF: saves state!

	RenderSrv->SetVertexLayout(FinalVertexLayout);

	for (int i = 0; i < TerrainObjects.Size(); ++i)
	{
		Scene::CTerrain* pTerrain = TerrainObjects[i];

		DWORD PatchCount = 0, QuarterPatchCount = 0;

	// Collect patches and quarterpatches to instance buffer, remember offset and counts
		//???patches from begin, quarterpatches from end? or two instance buffers? or tmp instance storage?

		//matrix44* pInstData = (matrix44*)InstanceBuffer->Map(Map_WriteDiscard);
		//n_assert_dbg(pInstData);
		//for (int InstIdx = i; InstIdx < j; ++InstIdx)
		//	*pInstData++ = Models[InstIdx].pModel->GetNode()->GetWorldMatrix();
		//InstanceBuffer->Unmap();

	// Apply not-instanced CDLOD shader vars for the batch
		Shader->SetTexture(hHeightMap, *pTerrain->GetHeightMap());

		if (i == 0) Shader->BeginPass(0);
		else Shader->CommitChanges(); //???!!!check ShaderVarsChanged?!

		if (PatchCount)
		{
			CMesh* pPatch = GetPatchMesh(pTerrain->GetPatchSize());
			RenderSrv->SetInstanceBuffer(1, InstanceBuffer, PatchCount);
			RenderSrv->SetVertexBuffer(0, pPatch->GetVertexBuffer());
			RenderSrv->SetIndexBuffer(pPatch->GetIndexBuffer());
			RenderSrv->SetPrimitiveGroup(pPatch->GetGroup(0));
			RenderSrv->Draw();
		}

		if (QuarterPatchCount)
		{
			CMesh* pPatch = GetPatchMesh(pTerrain->GetPatchSize() << 1);
			RenderSrv->SetInstanceBuffer(1, InstanceBuffer, QuarterPatchCount, MaxInstanceCount - QuarterPatchCount);
			RenderSrv->SetVertexBuffer(0, pPatch->GetVertexBuffer());
			RenderSrv->SetIndexBuffer(pPatch->GetIndexBuffer());
			RenderSrv->SetPrimitiveGroup(pPatch->GetGroup(0));
			RenderSrv->Draw();
		}
	}

	Shader->EndPass();
	Shader->End();

	TerrainObjects.Clear();
	pLights = NULL;
}
//---------------------------------------------------------------------

bool CTerrainRenderer::CreatePatchMesh(DWORD Size)
{
	if (!IsPow2(Size) || Size < 2) FAIL;

	nString PatchName;
	PatchName.Format("Patch%dx%d", Size, Size);
	PMesh Patch = RenderSrv->MeshMgr.GetTypedResource(CStrID(PatchName.Get()));
	if (!Patch->IsLoaded())
	{
		float InvEdgeSize = 1.f / (float)Size;
		DWORD VerticesPerEdge = Size + 1;
		DWORD VertexCount = VerticesPerEdge * VerticesPerEdge;
		n_assert(VertexCount <= 65535); // because of 16-bit index buffer

		PVertexBuffer VB = n_new(CVertexBuffer);
		if (!VB->Create(PatchVertexLayout, VertexCount, Usage_Immutable, CPU_NoAccess)) FAIL;
		vector3* pVBData = (vector3*)VB->Map(Map_Setup);
		for (DWORD z = 0; z < VerticesPerEdge; ++z)
			for (DWORD x = 0; x < VerticesPerEdge; ++x)
				pVBData[z * VerticesPerEdge + x].set(x * InvEdgeSize, 0.f, z * InvEdgeSize);
		VB->Unmap();

		//???use TriStrip?
		DWORD IndexCount = Size * Size * 6;

		PIndexBuffer IB = n_new(CIndexBuffer);
		if (!IB->Create(CIndexBuffer::Index16, IndexCount, Usage_Immutable, CPU_NoAccess)) FAIL;
		ushort* pIBData = (ushort*)IB->Map(Map_Setup);
		for (DWORD z = 0; z < Size; ++z)
			for (DWORD x = 0; x < Size; ++x)
			{
				*pIBData++ = (ushort)(z * VerticesPerEdge + x);
				*pIBData++ = (ushort)(z * VerticesPerEdge + (x + 1));
				*pIBData++ = (ushort)((z + 1) * VerticesPerEdge + x);
				*pIBData++ = (ushort)(z * VerticesPerEdge + (x + 1));
				*pIBData++ = (ushort)((z + 1) * VerticesPerEdge + (x + 1));
				*pIBData++ = (ushort)((z + 1) * VerticesPerEdge + x);
			}
		IB->Unmap();

		nArray<CMeshGroup> MeshGroups(1, 0);
		CMeshGroup& Group = *MeshGroups.Reserve(1);
		Group.Topology = TriList;
		Group.FirstVertex = 0;
		Group.VertexCount = VertexCount;
		Group.FirstIndex = 0;
		Group.IndexCount = IndexCount;
		Group.AABB.vmin = vector3::Zero;
		Group.AABB.vmax.set(1.f, 0.f, 1.f);

		if (!Patch->Setup(VB, IB, MeshGroups)) FAIL;
	}

	PatchMeshes.Add(Size, Patch.get());
	OK;
}
//---------------------------------------------------------------------

CMesh* CTerrainRenderer::GetPatchMesh(DWORD Size)
{
	int Idx = PatchMeshes.FindIndex(Size);
	if (Idx == INVALID_INDEX)
	{
		n_assert(CreatePatchMesh(Size));
		Idx = PatchMeshes.FindIndex(Size);
	}
	return PatchMeshes.ValueAtIndex(Idx);
}
//---------------------------------------------------------------------

}