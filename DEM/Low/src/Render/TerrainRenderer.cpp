#include "TerrainRenderer.h"
#include <Render/GPUDriver.h>
#include <Render/Terrain.h>
#include <Render/CDLODData.h>
#include <Render/Light.h>
#include <Render/Mesh.h>
#include <Render/Material.h>
#include <Render/Effect.h>
#include <Render/Texture.h>
#include <Render/Sampler.h>
#include <Core/Factory.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CTerrainRenderer, 'TRNR', Render::IRenderer);

CTerrainRenderer::CTerrainRenderer() = default;
CTerrainRenderer::~CTerrainRenderer() = default;

bool CTerrainRenderer::Init(const Data::CParams& Params, CGPUDriver& GPU)
{
	CSamplerDesc HMSamplerDesc;
	HMSamplerDesc.SetDefaults();
	HMSamplerDesc.AddressU = TexAddr_Clamp;
	HMSamplerDesc.AddressV = TexAddr_Clamp;
	HMSamplerDesc.Filter = TexFilter_MinMag_Linear_Mip_Point;
	HeightMapSampler = GPU.CreateSampler(HMSamplerDesc);

	OK;
}
//---------------------------------------------------------------------

bool CTerrainRenderer::BeginRange(const CRenderContext& Context)
{
	// Skip terrain rendering. Can fall back to manual 4-sample filtering in a shader instead.
	if (!Context.pGPU->CheckCaps(Caps_VSTexFiltering_Linear)) return false;

	pCurrMaterial = nullptr;
	pCurrTech = nullptr;

	//???!!!can cache for each tech by tech index and don't search constants each frame?
	ConstVSCDLODParams = {};
	ConstGridConsts = {};
	ConstFirstInstanceIndex = {};
	ConstInstanceDataVS = {};
	ConstInstanceDataPS = {};
	ResourceHeightMap = {};
	ConstWorldMatrix = {};
	ConstLightIndices = {};

	return true;
}
//---------------------------------------------------------------------

//???!!!support rendering multiple terrains at once?! could be useful for open worlds and subdividing large levels!
void CTerrainRenderer::Render(const CRenderContext& Context, IRenderable& Renderable)
{
	CTerrain& Terrain = static_cast<CTerrain&>(Renderable);

	if (!Terrain.GetPatchMesh() || !Terrain.GetQuarterPatchMesh() || !Terrain.GetCDLODData()) return;

	if (Terrain.GetPatches().empty()) return;

	static const CStrID sidWorldMatrix("WorldMatrix");
	static const CStrID sidLightCount("LightCount");
	static const CStrID sidLightIndices("LightIndices");

	// Select tech for the maximal light count used per-patch

	UPTR LightCount = 0;
	const CTechnique* pTech = Context.pShaderTechCache[Terrain.ShaderTechIndex];
	const auto& Passes = pTech->GetPasses(LightCount);
	if (Passes.empty()) return;

	// Apply material, if changed

	auto pMaterial = Terrain.Material.Get();
	if (pMaterial != pCurrMaterial)
	{
		n_assert_dbg(pMaterial);
		n_verify_dbg(pMaterial->Apply());
		pCurrMaterial = pMaterial;
	}

	// Pass tech params to GPU

	CGPUDriver& GPU = *Context.pGPU;
	if (pTech != pCurrTech)
	{
		const CShaderParamTable& ParamTable = pTech->GetParamTable();

		ConstInstanceDataVS = ParamTable.GetConstant(CStrID("InstanceDataVS"));
		if (!ConstInstanceDataVS) return;

		ConstVSCDLODParams = ParamTable.GetConstant(CStrID("VSCDLODParams"));
		ConstGridConsts = ParamTable.GetConstant(CStrID("GridConsts"));
		ConstFirstInstanceIndex = ParamTable.GetConstant(CStrID("FirstInstanceIndex"));
		ResourceHeightMap = ParamTable.GetResource(CStrID("HeightMapVS"));

		ConstInstanceDataPS = ParamTable.GetConstant(CStrID("InstanceDataPS"));
		if (ConstInstanceDataPS)
			ConstLightIndices = ConstInstanceDataPS[0][sidLightIndices];

		if (auto pVSLinearSampler = ParamTable.GetSampler(CStrID("VSLinearSampler")))
			pVSLinearSampler->Apply(GPU, HeightMapSampler.Get());

		pCurrTech = pTech;
	}

	if (ResourceHeightMap)
		ResourceHeightMap->Apply(GPU, Terrain.GetHeightMap());

	CShaderParamStorage PerInstance(pTech->GetParamTable(), GPU);

	const CCDLODData& CDLOD = *Terrain.GetCDLODData();

	if (ConstVSCDLODParams)
	{
		struct alignas(16)
		{
			float WorldToHM[4];
			float TerrainYScale;
			float TerrainYOffset;
			float InvSplatSizeX;
			float InvSplatSizeZ;
			std::array<float, 4> NormalMapUVCoeffs = { 1.f, 1.f, 0.f, 0.f };
			std::array<float, 4> SplatMapUVCoeffs;
			float WorldMaxX;
			float WorldMaxZ;
		} CDLODParams;

		// Fill instance data with patches and quarter-patches to render

		CAABB AABB = CDLOD.GetAABB();
		const auto LocalSize = AABB.Size();
		AABB.Transform(Terrain.Transform);
		const auto WorldSize = AABB.Size();

		const float HMTextelWidth = 1.f / static_cast<float>(CDLOD.GetHeightMapWidth());
		const float HMTextelHeight = 1.f / static_cast<float>(CDLOD.GetHeightMapHeight());

		// Map texels to vertices exactly. UV will be in range (0.5 * texelsize, 1 - 0.5 * texelsize).
		CDLODParams.WorldToHM[0] = (1.f - HMTextelWidth) / WorldSize.x;
		CDLODParams.WorldToHM[1] = (1.f - HMTextelHeight) / WorldSize.z;
		CDLODParams.WorldToHM[2] = -AABB.Min.x * CDLODParams.WorldToHM[0] + HMTextelWidth * 0.5f;
		CDLODParams.WorldToHM[3] = -AABB.Min.z * CDLODParams.WorldToHM[1] + HMTextelHeight * 0.5f;

		const float ScaleY = CDLOD.GetVerticalScale() * WorldSize.y / LocalSize.y;
		CDLODParams.TerrainYScale = 65535.f * ScaleY;
		CDLODParams.TerrainYOffset = -32767.f * ScaleY + Terrain.Transform.Translation().y;
		CDLODParams.InvSplatSizeX = Terrain.GetInvSplatSizeX();
		CDLODParams.InvSplatSizeZ = Terrain.GetInvSplatSizeZ();
		if (const auto& Tex = pCurrMaterial->GetValues().GetResource(CStrID("TexGeometryNormalMap")))
		{
			// Block compressed textures add extra rows & columns, breaking exact texel -> vertex mapping
			// TODO: can make better?
			const auto& Desc = Tex->GetDesc();
			CDLODParams.NormalMapUVCoeffs[0] = static_cast<float>(CDLOD.GetHeightMapWidth()) / static_cast<float>(Desc.Width);
			CDLODParams.NormalMapUVCoeffs[1] = static_cast<float>(CDLOD.GetHeightMapHeight()) / static_cast<float>(Desc.Height);
		}
		CDLODParams.SplatMapUVCoeffs = CDLOD.GetSplatMapUVCoeffs();
		CDLODParams.WorldMaxX = AABB.Max.x;
		CDLODParams.WorldMaxZ = AABB.Max.z;

		PerInstance.SetRawConstant(ConstVSCDLODParams, CDLODParams);
	}

	//!!!implement looping if instance buffer is too small!
	UPTR MaxInstanceCountConst = ConstInstanceDataVS.GetElementCount();
	if (ConstInstanceDataPS)
	{
		const UPTR MaxInstanceCountConstPS = ConstInstanceDataPS.GetElementCount();
		if (MaxInstanceCountConst < MaxInstanceCountConstPS)
			MaxInstanceCountConst = MaxInstanceCountConstPS;
	}
	n_assert_dbg(MaxInstanceCountConst > 1);

	//!!!implement looping if instance buffer is too small!
	n_assert_dbg(MaxInstanceCountConst >= Terrain.GetPatches().size());

	const auto TechLightCount = std::min(CTerrain::MAX_LIGHTS_PER_PATCH, ConstLightIndices.GetTotalComponentCount());
	const bool UploadLightInfo = ConstInstanceDataPS && TechLightCount;

	//!!!FIXME: tmp!
	struct CRec
	{
		acl::Vector4_32 ScaleOffset;
		acl::Vector4_32 MorphConsts;
	};

	//???PERF: optimize uploading? use paddings to maintain align16?
	const auto FullInstanceCount = Terrain.GetFullPatchCount();
	UPTR FullInstanceIndex = 0;
	UPTR QuarterInstanceIndex = FullInstanceCount;
	for (const auto& CurrPatch : Terrain.GetPatches())
	{
		const auto Index = CurrPatch.IsFullPatch ? FullInstanceIndex++ : QuarterInstanceIndex++;

		// Setup instance patch constants
		const CRec Rec{ CurrPatch.ScaleOffset, acl::vector_set(Terrain.LODParams[CurrPatch.LOD].Morph1, Terrain.LODParams[CurrPatch.LOD].Morph2, 0.f, 0.f) };
		PerInstance.SetRawConstant(ConstInstanceDataVS[Index], Rec);

		// Setup instance lights
		if (UploadLightInfo)
		{
			CShaderConstantParam CurrInstanceDataPS = ConstInstanceDataPS[Index];
			CShaderConstantParam CurrLightIndices = CurrInstanceDataPS[sidLightIndices];
			for (U32 i = 0; i < TechLightCount; ++i)
			{
				if (!CurrPatch.Lights[i])
				{
					PerInstance.SetInt(CurrLightIndices.GetComponent(i), -1);
					break;
				}

				if (CurrPatch.Lights[i]->GPUIndex != INVALID_INDEX_T<U32>)
					PerInstance.SetInt(CurrLightIndices.GetComponent(i), CurrPatch.Lights[i]->GPUIndex);
			}
		}
	}

	// Render patches //!!!may collect patches of different CTerrains if material is the same and instance buffer is big enough!

	if (FullInstanceCount)
	{
		if (ConstGridConsts)
		{
			float GridConsts[2];
			GridConsts[0] = CDLOD.GetPatchSize() * 0.5f;
			GridConsts[1] = 1.f / GridConsts[0];
			PerInstance.SetRawConstant(ConstGridConsts, GridConsts);
		}

		PerInstance.SetUInt(ConstFirstInstanceIndex, 0);

		PerInstance.Apply();

		const CMesh* pMesh = Terrain.GetPatchMesh();
		n_assert_dbg(pMesh);
		CVertexBuffer* pVB = pMesh->GetVertexBuffer().Get();
		n_assert_dbg(pVB);

		GPU.SetVertexLayout(pVB->GetVertexLayout());
		GPU.SetVertexBuffer(0, pVB);
		GPU.SetIndexBuffer(pMesh->GetIndexBuffer().Get());

		const CPrimitiveGroup* pGroup = pMesh->GetGroup(0);
		for (const auto& Pass : Passes)
		{
			GPU.SetRenderState(Pass);
			GPU.DrawInstanced(*pGroup, FullInstanceCount);
		}
	}

	if (QuarterInstanceIndex > FullInstanceCount)
	{
		if (ConstGridConsts)
		{
			float GridConsts[2];
			GridConsts[0] = CDLOD.GetPatchSize() * 0.25f;
			GridConsts[1] = 1.f / GridConsts[0];
			PerInstance.SetRawConstant(ConstGridConsts, GridConsts);
		}

		PerInstance.SetUInt(ConstFirstInstanceIndex, FullInstanceCount);

		PerInstance.Apply();

		const CMesh* pMesh = Terrain.GetQuarterPatchMesh();
		n_assert_dbg(pMesh);
		CVertexBuffer* pVB = pMesh->GetVertexBuffer().Get();
		n_assert_dbg(pVB);

		GPU.SetVertexLayout(pVB->GetVertexLayout());
		GPU.SetVertexBuffer(0, pVB);
		GPU.SetIndexBuffer(pMesh->GetIndexBuffer().Get());

		const CPrimitiveGroup* pGroup = pMesh->GetGroup(0);
		for (const auto& Pass : Passes)
		{
			GPU.SetRenderState(Pass);
			GPU.DrawInstanced(*pGroup, QuarterInstanceIndex - FullInstanceCount);
		}
	}
}
//---------------------------------------------------------------------

}
