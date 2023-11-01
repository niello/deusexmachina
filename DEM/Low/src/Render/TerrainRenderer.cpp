#include "TerrainRenderer.h"
#include <Render/GPUDriver.h>
#include <Render/Terrain.h>
#include <Render/CDLODData.h>
#include <Render/Light.h>
#include <Render/Mesh.h>
#include <Render/Material.h>
#include <Render/Effect.h>
#include <Render/Sampler.h>
#include <Math/Sphere.h>
#include <Data/Params.h>
#include <Core/Factory.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CTerrainRenderer, 'TRNR', Render::IRenderer);

CTerrainRenderer::CTerrainRenderer() = default;

CTerrainRenderer::~CTerrainRenderer()
{
	if (pInstances) n_free_aligned(pInstances);
}
//---------------------------------------------------------------------

bool CTerrainRenderer::Init(const Data::CParams& Params, CGPUDriver& GPU)
{
	CSamplerDesc HMSamplerDesc;
	HMSamplerDesc.SetDefaults();
	HMSamplerDesc.AddressU = TexAddr_Clamp;
	HMSamplerDesc.AddressV = TexAddr_Clamp;
	HMSamplerDesc.Filter = TexFilter_MinMag_Linear_Mip_Point;
	HeightMapSampler = GPU.CreateSampler(HMSamplerDesc);

	MaxInstanceCount = std::max(0, Params.Get<int>(CStrID("InstanceVBSize"), 512));

	//!!!PERF: for D3D11 const instancing can create CB without a RAM copy and update whole!
	pInstances = (CPatchInstance*)n_malloc_aligned(sizeof(CPatchInstance) * MaxInstanceCount, 16);

	OK;
}
//---------------------------------------------------------------------

bool CTerrainRenderer::CheckNodeSphereIntersection(const CLightTestArgs& Args, const sphere& Sphere, U32 X, U32 Z, U32 LOD, UPTR& AABBTestCounter)
{
	I16 MinY, MaxY;
	Args.pCDLOD->GetMinMaxHeight(X, Z, LOD, MinY, MaxY);

	// Node has no data, can't intersect with it
	if (MaxY < MinY) FAIL;

	U32 NodeSize = Args.pCDLOD->GetPatchSize() << LOD;
	float ScaleX = NodeSize * Args.ScaleBase.x;
	float ScaleZ = NodeSize * Args.ScaleBase.z;
	float NodeMinX = Args.AABBMinX + X * ScaleX;
	float NodeMinZ = Args.AABBMinZ + Z * ScaleZ;

	CAABB NodeAABB;
	NodeAABB.Min.x = NodeMinX;
	NodeAABB.Min.y = MinY * Args.ScaleBase.y;
	NodeAABB.Min.z = NodeMinZ;
	NodeAABB.Max.x = NodeMinX + ScaleX;
	NodeAABB.Max.y = MaxY * Args.ScaleBase.y;
	NodeAABB.Max.z = NodeMinZ + ScaleZ;

	if (Sphere.GetClipStatus(NodeAABB) == EClipStatus::Outside) FAIL;

	// Leaf node reached
	if (LOD == 0) OK;

	++AABBTestCounter;
	if (AABBTestCounter >= LIGHT_INTERSECTION_COARSE_TEST_MAX_AABBS) OK;

	const U32 XNext = X << 1, ZNext = Z << 1, NextLOD = LOD - 1;

	if (CheckNodeSphereIntersection(Args, Sphere, XNext, ZNext, NextLOD, AABBTestCounter)) OK;
	if (Args.pCDLOD->HasNode(XNext + 1, ZNext, NextLOD) && CheckNodeSphereIntersection(Args, Sphere, XNext + 1, ZNext, NextLOD, AABBTestCounter)) OK;
	if (Args.pCDLOD->HasNode(XNext, ZNext + 1, NextLOD) && CheckNodeSphereIntersection(Args, Sphere, XNext, ZNext + 1, NextLOD, AABBTestCounter)) OK;
	if (Args.pCDLOD->HasNode(XNext + 1, ZNext + 1, NextLOD) && CheckNodeSphereIntersection(Args, Sphere, XNext + 1, ZNext + 1, NextLOD, AABBTestCounter)) OK;

	// No child node touches the volume
	FAIL;
}
//---------------------------------------------------------------------

bool CTerrainRenderer::CheckNodeFrustumIntersection(const CLightTestArgs& Args, const matrix44& Frustum, U32 X, U32 Z, U32 LOD, UPTR& AABBTestCounter)
{
	I16 MinY, MaxY;
	Args.pCDLOD->GetMinMaxHeight(X, Z, LOD, MinY, MaxY);

	// Node has no data, can't intersect with it
	if (MaxY < MinY) FAIL;

	U32 NodeSize = Args.pCDLOD->GetPatchSize() << LOD;
	float ScaleX = NodeSize * Args.ScaleBase.x;
	float ScaleZ = NodeSize * Args.ScaleBase.z;
	float NodeMinX = Args.AABBMinX + X * ScaleX;
	float NodeMinZ = Args.AABBMinZ + Z * ScaleZ;

	CAABB NodeAABB;
	NodeAABB.Min.x = NodeMinX;
	NodeAABB.Min.y = MinY * Args.ScaleBase.y;
	NodeAABB.Min.z = NodeMinZ;
	NodeAABB.Max.x = NodeMinX + ScaleX;
	NodeAABB.Max.y = MaxY * Args.ScaleBase.y;
	NodeAABB.Max.z = NodeMinZ + ScaleZ;

	if (NodeAABB.GetClipStatus(Frustum) == EClipStatus::Outside) FAIL;

	// Leaf node reached
	if (LOD == 0) OK;

	++AABBTestCounter;
	if (AABBTestCounter >= LIGHT_INTERSECTION_COARSE_TEST_MAX_AABBS) OK;

	const U32 XNext = X << 1, ZNext = Z << 1, NextLOD = LOD - 1;

	if (CheckNodeFrustumIntersection(Args, Frustum, XNext, ZNext, NextLOD, AABBTestCounter)) OK;
	if (Args.pCDLOD->HasNode(XNext + 1, ZNext, NextLOD) && CheckNodeFrustumIntersection(Args, Frustum, XNext + 1, ZNext, NextLOD, AABBTestCounter)) OK;
	if (Args.pCDLOD->HasNode(XNext, ZNext + 1, NextLOD) && CheckNodeFrustumIntersection(Args, Frustum, XNext, ZNext + 1, NextLOD, AABBTestCounter)) OK;
	if (Args.pCDLOD->HasNode(XNext + 1, ZNext + 1, NextLOD) && CheckNodeFrustumIntersection(Args, Frustum, XNext + 1, ZNext + 1, NextLOD, AABBTestCounter)) OK;

	// No child node touches the volume
	FAIL;
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
	ConstLightCount = {};
	ConstLightIndices = {};

	return true;
}
//---------------------------------------------------------------------

//???!!!support rendering multiple terrains at once?! could be useful for open worlds and subdividing large levels!
void CTerrainRenderer::Render(const CRenderContext& Context, IRenderable& Renderable)
{
	CTerrain& Terrain = static_cast<CTerrain&>(Renderable);

	if (!Terrain.GetPatchMesh() || !Terrain.GetQuarterPatchMesh() || !Terrain.GetCDLODData()) return;

	if (Terrain.GetPatches().empty() && Terrain.GetQuarterPatches().empty()) return;

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
	UPTR TechLightCount;
	if (pTech != pCurrTech)
	{
		pCurrTech = pTech;

		const CShaderParamTable& ParamTable = pTech->GetParamTable();

		ConstVSCDLODParams = ParamTable.GetConstant(CStrID("VSCDLODParams"));
		ConstGridConsts = ParamTable.GetConstant(CStrID("GridConsts"));
		ConstFirstInstanceIndex = ParamTable.GetConstant(CStrID("FirstInstanceIndex"));
		ConstInstanceDataVS = ParamTable.GetConstant(CStrID("InstanceDataVS"));
		ConstInstanceDataPS = ParamTable.GetConstant(CStrID("InstanceDataPS"));
		ResourceHeightMap = ParamTable.GetResource(CStrID("HeightMapVS"));

		TechLightCount = 0;
		if (ConstInstanceDataPS)
		{
			ConstLightIndices = ConstInstanceDataPS[0][sidLightIndices];
			TechLightCount = ConstLightIndices.GetTotalComponentCount();
		}

		if (auto pVSLinearSampler = ParamTable.GetSampler(CStrID("VSLinearSampler")))
			pVSLinearSampler->Apply(GPU, HeightMapSampler.Get());
	}

	if (ResourceHeightMap)
		ResourceHeightMap->Apply(GPU, Terrain.GetHeightMap());

	CShaderParamStorage PerInstance(pTech->GetParamTable(), GPU);

	const CCDLODData& CDLOD = *Terrain.GetCDLODData();

	if (ConstVSCDLODParams)
	{
		struct
		{
			float WorldToHM[4];
			float TerrainYScale;
			float TerrainYOffset;
			float InvSplatSizeX;
			float InvSplatSizeZ;
			float TexelSize[2];
			//float TextureSize[2]; // For manual bilinear filtering in VS
		} CDLODParams;

		// Fill instance data with patches and quarter-patches to render

		CAABB AABB = CDLOD.GetAABB();
		const auto LocalSize = AABB.Size();
		AABB.Transform(Terrain.Transform);
		const auto WorldSize = AABB.Size();

		const float ScaleY = WorldSize.y / LocalSize.y;

		CDLODParams.WorldToHM[0] = 1.f / WorldSize.x;
		CDLODParams.WorldToHM[1] = 1.f / WorldSize.z;
		CDLODParams.WorldToHM[2] = -AABB.Min.x * CDLODParams.WorldToHM[0];
		CDLODParams.WorldToHM[3] = -AABB.Min.z * CDLODParams.WorldToHM[1];
		CDLODParams.TerrainYScale = 65535.f * ScaleY;
		CDLODParams.TerrainYOffset = -32767.f * ScaleY + Terrain.Transform.Translation().y;
		CDLODParams.InvSplatSizeX = Terrain.GetInvSplatSizeX();
		CDLODParams.InvSplatSizeZ = Terrain.GetInvSplatSizeZ();
		CDLODParams.TexelSize[0] = 1.f / (float)CDLOD.GetHeightMapWidth();
		CDLODParams.TexelSize[1] = 1.f / (float)CDLOD.GetHeightMapHeight();

		PerInstance.SetRawConstant(ConstVSCDLODParams, &CDLODParams, sizeof(CDLODParams));
	}

	//!!!implement looping if instance buffer is too small!
	if (ConstInstanceDataVS)
	{
		UPTR MaxInstanceCountConst = ConstInstanceDataVS.GetElementCount();
		if (ConstInstanceDataPS)
		{
			const UPTR MaxInstanceCountConstPS = ConstInstanceDataPS.GetElementCount();
			if (MaxInstanceCountConst < MaxInstanceCountConstPS)
				MaxInstanceCountConst = MaxInstanceCountConstPS;
		}
		n_assert_dbg(MaxInstanceCountConst > 1);

		//!!!implement looping if instance buffer is too small!
		n_assert_dbg(MaxInstanceCountConst >= (Terrain.GetPatches().size() + Terrain.GetQuarterPatches().size()));

		const bool UploadLightInfo = ConstInstanceDataPS && TechLightCount;
		U32 AvailableLightCount = (LightCount == 0) ? TechLightCount : std::min(LightCount, TechLightCount);
		if (AvailableLightCount > INSTANCE_MAX_LIGHT_COUNT) AvailableLightCount = INSTANCE_MAX_LIGHT_COUNT;

		//???PERF: optimize uploading? use paddings to maintain align16?
		//???PERF: use 2 different CPatchInstance structures for stream and const instancing?
		UPTR InstanceCount = 0;
		for (const auto& CurrPatch : Terrain.GetPatches())
		{
			// Setup instance patch constants

			PerInstance.SetRawConstant(ConstInstanceDataVS[InstanceCount], &CurrPatch, 6 * sizeof(float));

			// Setup instance lights

			if (UploadLightInfo)
			{
				CShaderConstantParam CurrInstanceDataPS = ConstInstanceDataPS[InstanceCount];
				CShaderConstantParam CurrLightIndices = CurrInstanceDataPS[sidLightIndices];
				PerInstance.SetInt(CurrLightIndices.GetComponent(0), -1);
			}

			++InstanceCount;
		}

		for (const auto& CurrPatch : Terrain.GetQuarterPatches())
		{
			// Setup instance patch constants

			PerInstance.SetRawConstant(ConstInstanceDataVS[InstanceCount], &CurrPatch, 6 * sizeof(float));

			// Setup instance lights

			if (UploadLightInfo)
			{
				CShaderConstantParam CurrInstanceDataPS = ConstInstanceDataPS[InstanceCount];
				CShaderConstantParam CurrLightIndices = CurrInstanceDataPS[sidLightIndices];
				PerInstance.SetInt(CurrLightIndices.GetComponent(0), -1);
			}

			++InstanceCount;
		}
	}

	// Set vertex layout

	const CMesh* pMesh = Terrain.GetPatchMesh();
	n_assert_dbg(pMesh);
	CVertexBuffer* pVB = pMesh->GetVertexBuffer().Get();
	n_assert_dbg(pVB);

	GPU.SetVertexLayout(pVB->GetVertexLayout());

	// Render patches //!!!may collect patches of different CTerrains if material is the same and instance buffer is big enough!

	if (!Terrain.GetPatches().empty())
	{
		if (ConstGridConsts)
		{
			float GridConsts[2];
			GridConsts[0] = CDLOD.GetPatchSize() * 0.5f;
			GridConsts[1] = 1.f / GridConsts[0];
			PerInstance.SetRawConstant(ConstGridConsts, &GridConsts, sizeof(GridConsts));
		}

		PerInstance.SetUInt(ConstFirstInstanceIndex, 0);

		PerInstance.Apply();

		GPU.SetVertexBuffer(0, pVB);
		GPU.SetIndexBuffer(pMesh->GetIndexBuffer().Get());

		const CPrimitiveGroup* pGroup = pMesh->GetGroup(0);
		for (const auto& Pass : Passes)
		{
			GPU.SetRenderState(Pass);
			GPU.DrawInstanced(*pGroup, Terrain.GetPatches().size());
		}
	}

	if (!Terrain.GetQuarterPatches().empty())
	{
		if (ConstGridConsts)
		{
			float GridConsts[2];
			GridConsts[0] = CDLOD.GetPatchSize() * 0.25f;
			GridConsts[1] = 1.f / GridConsts[0];
			PerInstance.SetRawConstant(ConstGridConsts, &GridConsts, sizeof(GridConsts));
		}

		PerInstance.SetUInt(ConstFirstInstanceIndex, Terrain.GetPatches().size());

		PerInstance.Apply();

		pMesh = Terrain.GetQuarterPatchMesh();
		n_assert_dbg(pMesh);
		pVB = pMesh->GetVertexBuffer().Get();
		n_assert_dbg(pVB);

		GPU.SetVertexBuffer(0, pVB);
		GPU.SetIndexBuffer(pMesh->GetIndexBuffer().Get());

		const CPrimitiveGroup* pGroup = pMesh->GetGroup(0);
		for (const auto& Pass : Passes)
		{
			GPU.SetRenderState(Pass);
			GPU.DrawInstanced(*pGroup, Terrain.GetQuarterPatches().size());
		}
	}
}
//---------------------------------------------------------------------

void CTerrainRenderer::EndRange(const CRenderContext& Context)
{
}
//---------------------------------------------------------------------

}
