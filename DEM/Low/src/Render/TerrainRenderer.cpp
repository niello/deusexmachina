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
#include <Render/SamplerDesc.h>
#include <Core/Factory.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CTerrainRenderer, 'TRNR', Render::IRenderer);

CTerrainRenderer::CTerrainRenderer() = default;
CTerrainRenderer::~CTerrainRenderer() = default;

CTerrainRenderer::CTerrainTechInterface* CTerrainRenderer::GetTechInterface(const CTechnique* pTech)
{
	auto It = _TechInterfaces.find(pTech);
	if (It != _TechInterfaces.cend()) return &It->second;

	auto& TechInterface = _TechInterfaces[pTech];

	auto& ParamTable = pTech->GetParamTable();
	if (ParamTable.HasParams())
	{
		static const CStrID sidInstanceDataVS("InstanceDataVS");
		static const CStrID sidVSCDLODParams("VSCDLODParams");
		static const CStrID sidGridConsts("GridConsts");
		static const CStrID sidFirstInstanceIndex("FirstInstanceIndex");
		static const CStrID sidInstanceDataPS("InstanceDataPS");
		static const CStrID sidLightIndices("LightIndices");
		static const CStrID sidHeightMapVS("HeightMapVS");
		static const CStrID sidVSLinearSampler("VSLinearSampler");

		TechInterface.PerInstanceParams = CShaderParamStorage(ParamTable, *_pGPU);

		TechInterface.ConstInstanceDataVS = ParamTable.GetConstant(sidInstanceDataVS);
		TechInterface.ConstVSCDLODParams = ParamTable.GetConstant(sidVSCDLODParams);
		TechInterface.ConstGridConsts = ParamTable.GetConstant(sidGridConsts);
		TechInterface.ConstFirstInstanceIndex = ParamTable.GetConstant(sidFirstInstanceIndex);

		TechInterface.ConstInstanceDataPS = ParamTable.GetConstant(sidInstanceDataPS);
		if (auto Struct = TechInterface.ConstInstanceDataPS[0])
			TechInterface.MemberLightIndices = Struct[sidLightIndices];

		TechInterface.ResourceHeightMap = ParamTable.GetResource(sidHeightMapVS);
		TechInterface.VSLinearSampler = ParamTable.GetSampler(sidVSLinearSampler);
	}

	TechInterface.TechMaxInstanceCount = TechInterface.ConstInstanceDataVS.GetElementCount();
	if (TechInterface.ConstInstanceDataPS)
		TechInterface.TechMaxInstanceCount = std::min(TechInterface.TechMaxInstanceCount, TechInterface.ConstInstanceDataPS.GetElementCount());

	TechInterface.TechLightCount = std::min(CTerrain::MAX_LIGHTS_PER_PATCH, TechInterface.MemberLightIndices.GetTotalComponentCount());

	TechInterface.TechNeedsMaterial = pTech->GetEffect()->GetMaterialParamTable().HasParams();

	return &TechInterface;
}
//---------------------------------------------------------------------

bool CTerrainRenderer::Init(const Data::CParams& Params, CGPUDriver& GPU)
{
	CSamplerDesc HMSamplerDesc;
	HMSamplerDesc.SetDefaults();
	HMSamplerDesc.AddressU = TexAddr_Clamp;
	HMSamplerDesc.AddressV = TexAddr_Clamp;
	HMSamplerDesc.Filter = TexFilter_MinMag_Linear_Mip_Point;
	_HeightMapSampler = GPU.CreateSampler(HMSamplerDesc);

	OK;
}
//---------------------------------------------------------------------

bool CTerrainRenderer::BeginRange(const CRenderContext& Context)
{
	// Skip terrain rendering. Can fall back to manual 4-sample filtering in a shader instead.
	if (!Context.pGPU->CheckCaps(Caps_VSTexFiltering_Linear)) return false;

	_pCurrTech = nullptr;
	_pCurrTechInterface = nullptr;
	_pCurrMaterial = nullptr;
	_pGPU = Context.pGPU; // FIXME: could instead pass CRenderContext to accessing methods

	return true;
}
//---------------------------------------------------------------------

//???!!!support rendering multiple terrains at once?! could be useful for open worlds and subdividing large levels!
void CTerrainRenderer::Render(const CRenderContext& Context, IRenderable& Renderable)
{
	CTerrain& Terrain = static_cast<CTerrain&>(Renderable);

	if (!Terrain.GetPatchMesh() || !Terrain.GetQuarterPatchMesh() || !Terrain.GetCDLODData() || Terrain.GetPatches().empty()) return;

	// Select tech for the maximal light count used per-patch

	UPTR LightCount = 0;
	const CTechnique* pTech = Context.pShaderTechCache[Terrain.ShaderTechIndex];
	const auto& Passes = pTech->GetPasses(LightCount);
	if (Passes.empty()) return;

	auto pMaterial = Terrain.Material.Get();
	n_assert_dbg(pMaterial);
	if (!pMaterial) return;

	// Apply material, if changed

	if (pTech != _pCurrTech)
	{
		_pCurrTechInterface = GetTechInterface(pTech);

		if (!_pCurrTechInterface->ConstInstanceDataVS || !_pCurrTechInterface->ConstVSCDLODParams) return;

		_pCurrTech = pTech;

		if (_pCurrTechInterface->VSLinearSampler)
			_pCurrTechInterface->VSLinearSampler->Apply(*_pGPU, _HeightMapSampler.Get());
	}

	if (_pCurrTechInterface->TechNeedsMaterial && pMaterial != _pCurrMaterial)
	{
		_pCurrMaterial = pMaterial;
		n_verify_dbg(pMaterial->Apply());
	}

	// Pass tech params to GPU

	const CCDLODData& CDLOD = *Terrain.GetCDLODData();

	// VSCDLODParams - params for a vertex shader
	// TODO: precalculate and store in CTerrain?
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

		// FIXME: can do better?
		Math::CAABB AABB = CDLOD.GetAABB();
		const auto LocalSize = rtm::vector_mul(AABB.Extent, 2.f);
		AABB = Math::AABBFromOBB(AABB, rtm::matrix_cast(Terrain.Transform));
		const auto WorldSize = rtm::vector_mul(AABB.Extent, 2.f);

		const float HMTextelWidth = 1.f / static_cast<float>(CDLOD.GetHeightMapWidth());
		const float HMTextelHeight = 1.f / static_cast<float>(CDLOD.GetHeightMapHeight());

		const rtm::vector4f AABBNegMin = rtm::vector_neg(rtm::vector_sub(AABB.Center, AABB.Extent));
		const rtm::vector4f AABBMax = rtm::vector_add(AABB.Center, AABB.Extent);

		// Map texels to vertices exactly. UV will be in range (0.5 * texelsize, 1 - 0.5 * texelsize).
		CDLODParams.WorldToHM[0] = (1.f - HMTextelWidth) / rtm::vector_get_x(WorldSize);
		CDLODParams.WorldToHM[1] = (1.f - HMTextelHeight) / rtm::vector_get_z(WorldSize);
		CDLODParams.WorldToHM[2] = rtm::vector_get_x(AABBNegMin) * CDLODParams.WorldToHM[0] + HMTextelWidth * 0.5f;
		CDLODParams.WorldToHM[3] = rtm::vector_get_z(AABBNegMin) * CDLODParams.WorldToHM[1] + HMTextelHeight * 0.5f;

		const float ScaleY = CDLOD.GetVerticalScale() * rtm::vector_get_y(WorldSize) / rtm::vector_get_y(LocalSize);
		CDLODParams.TerrainYScale = 65535.f * ScaleY;
		CDLODParams.TerrainYOffset = -32767.f * ScaleY + rtm::vector_get_y(Terrain.Transform.w_axis);
		CDLODParams.InvSplatSizeX = Terrain.GetInvSplatSizeX();
		CDLODParams.InvSplatSizeZ = Terrain.GetInvSplatSizeZ();
		if (_pCurrMaterial)
		{
			if (const auto& Tex = _pCurrMaterial->GetValues().GetResource(CStrID("TexGeometryNormalMap")))
			{
				// Block compressed textures add extra rows & columns, breaking exact texel -> vertex mapping
				// TODO: can make better?
				const auto& Desc = Tex->GetDesc();
				CDLODParams.NormalMapUVCoeffs[0] = static_cast<float>(CDLOD.GetHeightMapWidth()) / static_cast<float>(Desc.Width);
				CDLODParams.NormalMapUVCoeffs[1] = static_cast<float>(CDLOD.GetHeightMapHeight()) / static_cast<float>(Desc.Height);
			}
		}
		CDLODParams.SplatMapUVCoeffs = CDLOD.GetSplatMapUVCoeffs();
		CDLODParams.WorldMaxX = rtm::vector_get_x(AABBMax);
		CDLODParams.WorldMaxZ = rtm::vector_get_z(AABBMax);

		_pCurrTechInterface->PerInstanceParams.SetRawConstant(_pCurrTechInterface->ConstVSCDLODParams, CDLODParams);
	}

	// Heightmap texture for a vertex shader
	if (_pCurrTechInterface->ResourceHeightMap)
		_pCurrTechInterface->ResourceHeightMap->Apply(*_pGPU, Terrain.GetHeightMap());

	// Terrain patch instances and their affecting lights
	//!!!TODO: implement looping if instance buffer is too small! batch terrain clusters with identical material!
	n_assert_dbg(_pCurrTechInterface->TechMaxInstanceCount >= Terrain.GetPatches().size());
	const auto FullInstanceCount = Terrain.GetFullPatchCount();
	UPTR FullInstanceIndex = 0;
	UPTR QuarterInstanceIndex = FullInstanceCount;
	auto ConstVSInstance = _pCurrTechInterface->ConstInstanceDataVS[0];
	std::array<U32, CTerrain::MAX_LIGHTS_PER_PATCH> LightIndexBuffer;
	for (const auto& CurrPatch : Terrain.GetPatches())
	{
		const auto Index = CurrPatch.IsFullPatch ? FullInstanceIndex++ : QuarterInstanceIndex++;

		//!!!FIXME: tmp! better is to prepare the buffer in advance and don't refill it every frame!
		struct CRec
		{
			rtm::vector4f ScaleOffset;
			rtm::vector4f MorphConsts;
		};

		// Setup instance patch constants
		const CRec Rec{ CurrPatch.ScaleOffset, rtm::vector_set(Terrain.LODParams[CurrPatch.LOD].Morph1, Terrain.LODParams[CurrPatch.LOD].Morph2, 0.f, 0.f) };
		ConstVSInstance.Shift(_pCurrTechInterface->ConstInstanceDataVS, Index);
		_pCurrTechInterface->PerInstanceParams.SetRawConstant(ConstVSInstance, Rec);

		// Setup instance lights
		if (_pCurrTechInterface->TechLightCount)
		{
			_pCurrTechInterface->MemberLightIndices.Shift(_pCurrTechInterface->ConstInstanceDataPS, Index);
			U32 LightCount = 0;
			for (U32 i = 0; i < _pCurrTechInterface->TechLightCount; ++i)
			{
				if (!CurrPatch.Lights[i])
				{
					LightIndexBuffer[LightCount++] = -1;
					break;
				}

				if (CurrPatch.Lights[i]->GPUIndex != INVALID_INDEX_T<U32>)
					LightIndexBuffer[LightCount++] = CurrPatch.Lights[i]->GPUIndex;
			}
			_pCurrTechInterface->PerInstanceParams.SetRawConstant(_pCurrTechInterface->MemberLightIndices, LightIndexBuffer.data(), sizeof(U32) * LightCount);
		}
	}

	// Render patches

	if (FullInstanceCount)
	{
		if (_pCurrTechInterface->ConstGridConsts)
		{
			float GridConsts[2];
			GridConsts[0] = CDLOD.GetPatchSize() * 0.5f;
			GridConsts[1] = 1.f / GridConsts[0];
			_pCurrTechInterface->PerInstanceParams.SetRawConstant(_pCurrTechInterface->ConstGridConsts, GridConsts);
		}

		_pCurrTechInterface->PerInstanceParams.SetUInt(_pCurrTechInterface->ConstFirstInstanceIndex, 0);

		_pCurrTechInterface->PerInstanceParams.Apply();

		const CMesh* pMesh = Terrain.GetPatchMesh();
		CVertexBuffer* pVB = pMesh->GetVertexBuffer().Get();
		_pGPU->SetVertexLayout(pVB->GetVertexLayout());
		_pGPU->SetVertexBuffer(0, pVB);
		_pGPU->SetIndexBuffer(pMesh->GetIndexBuffer().Get());

		const CPrimitiveGroup* pGroup = pMesh->GetGroup(0);
		for (const auto& Pass : Passes)
		{
			_pGPU->SetRenderState(Pass);
			_pGPU->DrawInstanced(*pGroup, FullInstanceCount);
		}
	}

	if (QuarterInstanceIndex > FullInstanceCount)
	{
		if (_pCurrTechInterface->ConstGridConsts)
		{
			float GridConsts[2];
			GridConsts[0] = CDLOD.GetPatchSize() * 0.25f;
			GridConsts[1] = 1.f / GridConsts[0];
			_pCurrTechInterface->PerInstanceParams.SetRawConstant(_pCurrTechInterface->ConstGridConsts, GridConsts);
		}

		_pCurrTechInterface->PerInstanceParams.SetUInt(_pCurrTechInterface->ConstFirstInstanceIndex, FullInstanceCount);

		_pCurrTechInterface->PerInstanceParams.Apply();

		const CMesh* pMesh = Terrain.GetQuarterPatchMesh();
		CVertexBuffer* pVB = pMesh->GetVertexBuffer().Get();
		_pGPU->SetVertexLayout(pVB->GetVertexLayout());
		_pGPU->SetVertexBuffer(0, pVB);
		_pGPU->SetIndexBuffer(pMesh->GetIndexBuffer().Get());

		const CPrimitiveGroup* pGroup = pMesh->GetGroup(0);
		for (const auto& Pass : Passes)
		{
			_pGPU->SetRenderState(Pass);
			_pGPU->DrawInstanced(*pGroup, QuarterInstanceIndex - FullInstanceCount);
		}
	}
}
//---------------------------------------------------------------------

}
