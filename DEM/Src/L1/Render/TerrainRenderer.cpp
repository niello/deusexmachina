#include "TerrainRenderer.h"

#include <Render/RenderNode.h>
#include <Render/GPUDriver.h>
#include <Render/Terrain.h>
#include <Render/Material.h>
#include <Render/Effect.h>
#include <Render/EffectConstSetValues.h>
#include <Math/Sphere.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CTerrainRenderer, 'TRNR', Render::IRenderer);

CTerrainRenderer::CTerrainRenderer(): pInstances(NULL)
{
	// Setup dynamic enumeration
	InputSet_CDLOD = RegisterShaderInputSetID(CStrID("CDLOD"));

	InstanceDataDecl.SetSize(2);

	// Patch offset and scale in XZ
	CVertexComponent& Cmp = InstanceDataDecl[0];
	Cmp.Semantic = VCSem_TexCoord;
	Cmp.UserDefinedName = NULL;
	Cmp.Index = 0;
	Cmp.Format = VCFmt_Float32_4;
	Cmp.Stream = INSTANCE_BUFFER_STREAM_INDEX;
	Cmp.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
	Cmp.PerInstanceData = true;

	// Morph constants
	Cmp = InstanceDataDecl[1];
	Cmp.Semantic = VCSem_TexCoord;
	Cmp.UserDefinedName = NULL;
	Cmp.Index = 1;
	Cmp.Format = VCFmt_Float32_2;
	Cmp.Stream = INSTANCE_BUFFER_STREAM_INDEX;
	Cmp.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
	Cmp.PerInstanceData = true;

	//!!!DBG TMP! //???where to define?
	MaxInstanceCount = 64;
}
//---------------------------------------------------------------------

CTerrainRenderer::~CTerrainRenderer()
{
	if (pInstances) n_free_aligned(pInstances);
}
//---------------------------------------------------------------------

bool CTerrainRenderer::PrepareNode(CRenderNode& Node, const CRenderNodeContext& Context)
{
	CTerrain* pTerrain = Node.pRenderable->As<CTerrain>();
	n_assert_dbg(pTerrain);

	CMaterial* pMaterial = pTerrain->GetMaterial(); //!!!Get by MaterialLOD!
	if (!pMaterial) FAIL;

	CEffect* pEffect = pMaterial->GetEffect();
	EEffectType EffType = pEffect->GetType();
	if (Context.pEffectOverrides)
		for (UPTR i = 0; i < Context.pEffectOverrides->GetCount(); ++i)
			if (Context.pEffectOverrides->KeyAt(i) == EffType)
			{
				pEffect = Context.pEffectOverrides->ValueAt(i).GetUnsafe();
				break;
			}

	if (!pEffect) FAIL;

	Node.pMaterial = pMaterial;
	Node.pTech = pEffect->GetTechByInputSet(InputSet_CDLOD);
	if (!Node.pTech) FAIL;

	Node.pMesh = pTerrain->GetPatchMesh();
	Node.pGroup = pTerrain->GetPatchMesh()->GetGroup(0, 0); // For sorting, different terrain objects with the same mesh will be rendered sequentially

	OK;
}
//---------------------------------------------------------------------

CTerrainRenderer::ENodeStatus CTerrainRenderer::ProcessTerrainNode(const CProcessTerrainNodeArgs& Args,
																   U32 X, U32 Z, U32 LOD, float LODRange,
																   U32& PatchCount, U32& QPatchCount, EClipStatus Clip)
{
	const CCDLODData* pCDLOD = Args.pTerrain->GetCDLODData();

	I16 MinY, MaxY;
	pCDLOD->GetMinMaxHeight(X, Z, LOD, MinY, MaxY);

	// Node has no data, skip it completely
	if (MaxY < MinY) return Node_Invisible;

	U32 NodeSize = pCDLOD->GetPatchSize() << LOD;
	float ScaleX = NodeSize * Args.ScaleBaseX;
	float ScaleZ = NodeSize * Args.ScaleBaseZ;
	float NodeMinX = Args.AABBMinX + X * ScaleX;
	float NodeMinZ = Args.AABBMinZ + Z * ScaleZ;

	CAABB NodeAABB;
	NodeAABB.Min.x = NodeMinX;
	NodeAABB.Min.y = MinY * pCDLOD->GetVerticalScale();
	NodeAABB.Min.z = NodeMinZ;
	NodeAABB.Max.x = NodeMinX + ScaleX;
	NodeAABB.Max.y = MaxY * pCDLOD->GetVerticalScale();
	NodeAABB.Max.z = NodeMinZ + ScaleZ;

	if (Clip == Clipped)
	{
		Clip = NodeAABB.GetClipStatus(*Args.pViewProj);
		if (Clip == Outside) return Node_Invisible;
	}

	// NB: Always must check the main frame camera, even if some special camera is used for intermediate rendering
	sphere LODSphere(*Args.pCameraPos, LODRange);
	if (LODSphere.GetClipStatus(NodeAABB) == Outside) return Node_NotInLOD;

	// Bits 0 to 3 - if set, add child[0 .. 3]
	U8 ChildFlags = 0;
	enum
	{
		Child_TopLeft		= 0x01,
		Child_TopRight		= 0x02,
		Child_BottomLeft	= 0x04,
		Child_BottomRight	= 0x08,
		Child_All			= (Child_TopLeft | Child_TopRight | Child_BottomLeft | Child_BottomRight)
	};

	bool IsVisible;

	if (LOD > 0)
	{
		// Hack, see original CDLOD code. LOD 0 range is 0.9 of what is expected.
		float NextLODRange = LODRange * ((LOD == 1) ? 0.45f : 0.5f);

		IsVisible = false;

		// NB: Always must check the main frame camera, even if some special camera is used for intermediate rendering
		sphere LODSphere(*Args.pCameraPos, NextLODRange);
		EClipStatus NextClip = LODSphere.GetClipStatus(NodeAABB);
		if (NextClip != Outside)
		{
			U32 XNext = X << 1, ZNext = Z << 1;

			ENodeStatus Status = ProcessTerrainNode(Args, XNext, ZNext, LOD - 1, NextLODRange, PatchCount, QPatchCount, NextClip);
			if (Status != Node_Invisible)
			{
				IsVisible = true;
				if (Status == Node_NotInLOD) ChildFlags |= Child_TopLeft;
			}

			if (pCDLOD->HasNode(XNext + 1, ZNext, LOD - 1))
			{
				Status = ProcessTerrainNode(Args, XNext + 1, ZNext, LOD - 1, NextLODRange, PatchCount, QPatchCount, NextClip);
				if (Status != Node_Invisible)
				{
					IsVisible = true;
					if (Status == Node_NotInLOD) ChildFlags |= Child_TopRight;
				}
			}

			if (pCDLOD->HasNode(XNext, ZNext + 1, LOD - 1))
			{
				Status = ProcessTerrainNode(Args, XNext, ZNext + 1, LOD - 1, NextLODRange, PatchCount, QPatchCount, NextClip);
				if (Status != Node_Invisible)
				{
					IsVisible = true;
					if (Status == Node_NotInLOD) ChildFlags |= Child_BottomLeft;
				}
			}

			if (pCDLOD->HasNode(XNext + 1, ZNext + 1, LOD - 1))
			{
				Status = ProcessTerrainNode(Args, XNext + 1, ZNext + 1, LOD - 1, NextLODRange, PatchCount, QPatchCount, NextClip);
				if (Status != Node_Invisible)
				{
					IsVisible = true;
					if (Status == Node_NotInLOD) ChildFlags |= Child_BottomRight;
				}
			}
		}

		if (!ChildFlags) return IsVisible ? Node_Processed : Node_Invisible;
	}
	else
	{
		ChildFlags = Child_All;
		IsVisible = true;
	}

	float* pLODMorphConsts = Args.pMorphConsts + 2 * LOD;

	if (ChildFlags & Child_All)
	{
		// Add whole patch
		n_assert(PatchCount + QPatchCount < Args.MaxInstanceCount);
		CPatchInstance& Patch = Args.pInstances[PatchCount];
		Patch.ScaleOffset[0] = NodeAABB.Max.x - NodeAABB.Min.x;
		Patch.ScaleOffset[1] = NodeAABB.Max.z - NodeAABB.Min.z;
		Patch.ScaleOffset[2] = NodeAABB.Min.x;
		Patch.ScaleOffset[3] = NodeAABB.Min.z;
		Patch.MorphConsts[0] = pLODMorphConsts[0];
		Patch.MorphConsts[1] = pLODMorphConsts[1];
		++PatchCount;
	}
	else
	{
		// Add quarterpatches

		float NodeMinX = NodeAABB.Min.x;
		float NodeMinZ = NodeAABB.Min.z;
		float HalfScaleX = (NodeAABB.Max.x - NodeAABB.Min.x) * 0.5f;
		float HalfScaleZ = (NodeAABB.Max.z - NodeAABB.Min.z) * 0.5f;

		if (ChildFlags & Child_TopLeft)
		{
			n_assert(PatchCount + QPatchCount < Args.MaxInstanceCount);
			CPatchInstance& Patch = Args.pInstances[Args.MaxInstanceCount - QPatchCount - 1];
			Patch.ScaleOffset[0] = HalfScaleX;
			Patch.ScaleOffset[1] = HalfScaleZ;
			Patch.ScaleOffset[2] = NodeMinX;
			Patch.ScaleOffset[3] = NodeMinZ;
			Patch.MorphConsts[0] = pLODMorphConsts[0];
			Patch.MorphConsts[1] = pLODMorphConsts[1];
			++QPatchCount;
		}

		if (ChildFlags & Child_TopRight)
		{
			n_assert(PatchCount + QPatchCount < Args.MaxInstanceCount);
			CPatchInstance& Patch = Args.pInstances[Args.MaxInstanceCount - QPatchCount - 1];
			Patch.ScaleOffset[0] = HalfScaleX;
			Patch.ScaleOffset[1] = HalfScaleZ;
			Patch.ScaleOffset[2] = NodeMinX + HalfScaleX;
			Patch.ScaleOffset[3] = NodeMinZ;
			Patch.MorphConsts[0] = pLODMorphConsts[0];
			Patch.MorphConsts[1] = pLODMorphConsts[1];
			++QPatchCount;
		}

		if (ChildFlags & Child_BottomLeft)
		{
			n_assert(PatchCount + QPatchCount < Args.MaxInstanceCount);
			CPatchInstance& Patch = Args.pInstances[Args.MaxInstanceCount - QPatchCount - 1];
			Patch.ScaleOffset[0] = HalfScaleX;
			Patch.ScaleOffset[1] = HalfScaleZ;
			Patch.ScaleOffset[2] = NodeMinX;
			Patch.ScaleOffset[3] = NodeMinZ + HalfScaleZ;
			Patch.MorphConsts[0] = pLODMorphConsts[0];
			Patch.MorphConsts[1] = pLODMorphConsts[1];
			++QPatchCount;
		}

		if (ChildFlags & Child_BottomRight)
		{
			n_assert(PatchCount + QPatchCount < Args.MaxInstanceCount);
			CPatchInstance& Patch = Args.pInstances[Args.MaxInstanceCount - QPatchCount - 1];
			Patch.ScaleOffset[0] = HalfScaleX;
			Patch.ScaleOffset[1] = HalfScaleZ;
			Patch.ScaleOffset[2] = NodeMinX + HalfScaleX;
			Patch.ScaleOffset[3] = NodeMinZ + HalfScaleZ;
			Patch.MorphConsts[0] = pLODMorphConsts[0];
			Patch.MorphConsts[1] = pLODMorphConsts[1];
			++QPatchCount;
		}
	}

// Morphing artifacts test (from the original CDLOD code)
/*
#ifdef _DEBUG
	if (LOD < Terrain.GetLODCount() - 1)
	{
		//!!!Always must check the Main camera!
		float MaxDistToCameraSq = NodeAABB.MaxDistFromPointSq(RenderSrv->GetCameraPosition());
		float MorphStart = MorphConsts[LOD + 1].Start;
		if (MaxDistToCameraSq > MorphStart * MorphStart)
			Sys::Error("Visibility distance is too small!");
	}
#endif
*/

	return Node_Processed;
}
//---------------------------------------------------------------------

CArray<CRenderNode>::CIterator CTerrainRenderer::Render(const CRenderContext& Context, CArray<CRenderNode>& RenderQueue, CArray<CRenderNode>::CIterator ItCurr)
{
	CGPUDriver& GPU = *Context.pGPU;

	CArray<CRenderNode>::CIterator ItEnd = RenderQueue.End();

	if (!GPU.CheckCaps(Caps_VSTexFiltering_Linear))
	{
		// Skip terrain rendering. Can fall back to manual 4-sample filtering in a shader instead.
		while (ItCurr != ItEnd)
		{
			if (ItCurr->pRenderer != this) return ItCurr;
			++ItCurr;
		}
		return ItEnd;
	}

	const CMaterial* pCurrMaterial = NULL;
	const CTechnique* pCurrTech = NULL;

	const CEffectConstant* pConstCDLODParams = NULL;

	//!!!
	//GPU.SetVertexLayout(pVLInstanced);

	while (ItCurr != ItEnd)
	{
		if (ItCurr->pRenderer != this) return ItCurr;

		CTerrain* pTerrain = ItCurr->pRenderable->As<CTerrain>();

		float VisibilityRange = 1000.f;
		float MorphStartRatio = 0.7f;

		const CCDLODData* pCDLOD = pTerrain->GetCDLODData();
		U32 LODCount = pCDLOD->GetLODCount();

		// Calculate morph constants

		//!!!PERF: may recalculate only when LODCount / VisibilityRange changes!
		float MorphStart = 0.f;
		float CurrVisRange = VisibilityRange / (float)(1 << (LODCount - 1));
		float* pMorphConsts = (float*)_malloca(sizeof(float) * 2 * LODCount);
		float* pCurrMorphConst = pMorphConsts;
		for (U32 j = 0; j < LODCount; ++j)
		{
			float MorphEnd = j ? CurrVisRange : CurrVisRange * 0.9f;
			MorphStart = MorphStart + (MorphEnd - MorphStart) * MorphStartRatio;
			MorphEnd = n_lerp(MorphEnd, MorphStart, 0.01f);
			float MorphConst2 = 1.0f / (MorphEnd - MorphStart);
			*pCurrMorphConst++ = MorphEnd * MorphConst2;
			*pCurrMorphConst++ = MorphConst2;
			CurrVisRange *= 2.f;
		}

		// Fill instance data with patches and quarter-patches to render

		U32 PatchCount = 0, QuarterPatchCount = 0;

		//!!!PERF: for D3D11 const instancing can create CB without a RAM copy and update whole!
		if (!pInstances)
		{
			pInstances = (CPatchInstance*)n_malloc_aligned(sizeof(CPatchInstance) * MaxInstanceCount, 16);
			n_assert_dbg(pInstances);
		}

		CAABB AABB;
		n_verify_dbg(pTerrain->GetLocalAABB(AABB, 0));
		AABB.Transform(ItCurr->Transform);
		float AABBMinX = AABB.Min.x;
		float AABBMinZ = AABB.Min.z;
		float AABBSizeX = AABB.Max.x - AABBMinX;
		float AABBSizeZ = AABB.Max.z - AABBMinZ;

		CProcessTerrainNodeArgs Args;
		Args.pTerrain = pTerrain;
		Args.pInstances = pInstances;
		Args.pMorphConsts = pMorphConsts;
		Args.pViewProj = &Context.ViewProjection;
		Args.pCameraPos = &Context.CameraPosition;
		Args.MaxInstanceCount = MaxInstanceCount;
		Args.AABBMinX = AABBMinX;
		Args.AABBMinZ = AABBMinZ;
		Args.ScaleBaseX = AABBSizeX / (float)(pCDLOD->GetHeightMapWidth() - 1);
		Args.ScaleBaseZ = AABBSizeZ / (float)(pCDLOD->GetHeightMapHeight() - 1);

		const U32 TopPatchesW = pCDLOD->GetTopPatchCountW();
		const U32 TopPatchesH = pCDLOD->GetTopPatchCountH();
		const U32 TopLOD = LODCount - 1;
		for (U32 Z = 0; Z < TopPatchesH; ++Z)
			for (U32 X = 0; X < TopPatchesW; ++X)
				ProcessTerrainNode(Args, X, Z, TopLOD, VisibilityRange, PatchCount, QuarterPatchCount);

		_freea(pMorphConsts);

		if (!PatchCount && !QuarterPatchCount)
		{
			++ItCurr;
			continue;
		}

		// Sort patches

		//...
		//!!!can sort by distance (min dist to camera) before rendering!
		//or can sort by LOD!
		//if so, don't write instance data into the video memory directly,
		//write to tmp storage (with additional fields for sorting), sort, then send to GPU
		//...

		// Setup lights //???here or in ProcessTerrainNode()?

		UPTR LightCount = 0;
		//...

		// Apply material, if changed

		const CMaterial* pMaterial = ItCurr->pMaterial;
		if (pMaterial != pCurrMaterial)
		{
			n_assert_dbg(pMaterial);
			n_verify_dbg(pMaterial->Apply(GPU));
			pCurrMaterial = pMaterial;

			//!!!DBG TMP!
			Sys::DbgOut("Material changed: 0x%X\n", pMaterial);
		}

		// Pass consts and instance data to GPU

		const CTechnique* pTech = ItCurr->pTech;
		if (pTech != pCurrTech)
		{
			pConstCDLODParams = pTech->GetConstant(CStrID("CDLODParams"));
			pCurrTech = pTech;

			//!!!DBG TMP!
			Sys::DbgOut("Tech params requested by ID\n");
		}

		CEffectConstSetValues PerInstanceConstValues;
		PerInstanceConstValues.SetGPU(&GPU);

		if (pConstCDLODParams)
		{
			struct
			{
				float WorldToHM[4];
				float TerrainYScale;
				float TerrainYOffset;
				float InvSplatSizeX;
				float InvSplatSizeZ;
				float GridHalfSize;		//!!!changes for quarter patches!
				float InvGridHalfSize;	//!!!changes for quarter patches!
				float TexelSize[2];
			} CDLODParams;

			CDLODParams.WorldToHM[0] = 1.f / AABBSizeX;
			CDLODParams.WorldToHM[1] = 1.f / AABBSizeZ;
			CDLODParams.WorldToHM[2] = -AABBMinX * CDLODParams.WorldToHM[0];
			CDLODParams.WorldToHM[3] = -AABBMinZ * CDLODParams.WorldToHM[1];
			CDLODParams.TerrainYScale = 65535.f * pCDLOD->GetVerticalScale();
			CDLODParams.TerrainYOffset = -32767.f * pCDLOD->GetVerticalScale() + ItCurr->Transform.m[3][1]; // [3][1] = Translation.y
			CDLODParams.InvSplatSizeX = pTerrain->GetInvSplatSizeX();
			CDLODParams.InvSplatSizeZ = pTerrain->GetInvSplatSizeZ();
			CDLODParams.GridHalfSize = pCDLOD->GetPatchSize() * 0.5f;
			CDLODParams.InvGridHalfSize = 1.f / CDLODParams.GridHalfSize;
			CDLODParams.TexelSize[0] = 1.f / (float)pCDLOD->GetHeightMapWidth();
			CDLODParams.TexelSize[1] = 1.f / (float)pCDLOD->GetHeightMapHeight();

			PerInstanceConstValues.RegisterConstantBuffer(pConstCDLODParams->Desc.BufferHandle, NULL);
			PerInstanceConstValues.SetConstantValue(pConstCDLODParams, 0, &CDLODParams, sizeof(CDLODParams));
		}

		PerInstanceConstValues.ApplyConstantBuffers();

		//...
		//!!!if VB instancing and no instance buffer, create instance buffer!
		//!!!upload data to instance buffer!
		//...

		// Render patches //!!!may collect patches of different CTerrains if material is the same and instance buffer is big enough!

		const CPassList* pPasses = pTech->GetPasses(LightCount);
		n_assert_dbg(pPasses); // To test if it could happen at all
		if (!pPasses)
		{
			++ItCurr;
			continue;
		}

		/* render patches, then quarters!
		for (UPTR i = 0; i < pPasses->GetCount(); ++i)
		{
			GPU.SetRenderState((*pPasses)[i]);
			GPU.Draw(*pGroup);
		}
		*/

		//!!!DBG TMP!
		Sys::DbgOut("CTerrain rendered: %d patches, %d quarterpatches\n", PatchCount, QuarterPatchCount);

		++ItCurr;
	};

	return ItEnd;
}
//---------------------------------------------------------------------

}