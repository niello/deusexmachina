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

	HMSamplerDesc.SetDefaults();
	HMSamplerDesc.AddressU = TexAddr_Clamp;
	HMSamplerDesc.AddressV = TexAddr_Clamp;
	HMSamplerDesc.Filter = TexFilter_MinMag_Linear_Mip_Point;

	InstanceDataDecl.SetSize(2);

	// Patch offset and scale in XZ
	CVertexComponent* pCmp = &InstanceDataDecl[0];
	pCmp->Semantic = VCSem_TexCoord;
	pCmp->UserDefinedName = NULL;
	pCmp->Index = 0;
	pCmp->Format = VCFmt_Float32_4;
	pCmp->Stream = INSTANCE_BUFFER_STREAM_INDEX;
	pCmp->OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
	pCmp->PerInstanceData = true;

	// Morph constants
	pCmp = &InstanceDataDecl[1];
	pCmp->Semantic = VCSem_TexCoord;
	pCmp->UserDefinedName = NULL;
	pCmp->Index = 1;
	pCmp->Format = VCFmt_Float32_2;
	pCmp->Stream = INSTANCE_BUFFER_STREAM_INDEX;
	pCmp->OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
	pCmp->PerInstanceData = true;

	//!!!DBG TMP! //???where to define?
	MaxInstanceCount = 128;
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
	Node.pEffect = pEffect;
	Node.pTech = pEffect->GetTechByInputSet(InputSet_CDLOD);
	if (!Node.pTech) FAIL;

	Node.pMesh = pTerrain->GetPatchMesh();
	Node.pGroup = pTerrain->GetPatchMesh()->GetGroup(0, 0); // For sorting, different terrain objects with the same mesh will be rendered sequentially

	OK;
}
//---------------------------------------------------------------------

//!!!recalculation required only on viewer's position / look vector change!
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
		// NB: Visibility is tested for the current camera, NOT always for the main
		Clip = NodeAABB.GetClipStatus(*Args.pViewProj);
		if (Clip == Outside) return Node_Invisible;
	}

	// NB: Always must check the main frame camera, even if some special camera is used for intermediate rendering
	sphere LODSphere(*Args.pCameraPos, LODRange);
	if (LODSphere.GetClipStatus(NodeAABB) == Outside) return Node_NotInLOD;

	// Bits 0 to 3 - if set, add quarterpatch for child[0 .. 3]
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
		if (LODSphere.GetClipStatus(NodeAABB) == Outside)
		{
			// Add the whole node to the current LOD
			ChildFlags = Child_All;
		}
		else
		{
			const U32 XNext = X << 1, ZNext = Z << 1, NextLOD = LOD - 1;

			ENodeStatus Status = ProcessTerrainNode(Args, XNext, ZNext, NextLOD, NextLODRange, PatchCount, QPatchCount, Clip);
			if (Status != Node_Invisible)
			{
				IsVisible = true;
				if (Status == Node_NotInLOD) ChildFlags |= Child_TopLeft;
			}

			if (pCDLOD->HasNode(XNext + 1, ZNext, NextLOD))
			{
				Status = ProcessTerrainNode(Args, XNext + 1, ZNext, NextLOD, NextLODRange, PatchCount, QPatchCount, Clip);
				if (Status != Node_Invisible)
				{
					IsVisible = true;
					if (Status == Node_NotInLOD) ChildFlags |= Child_TopRight;
				}
			}

			if (pCDLOD->HasNode(XNext, ZNext + 1, NextLOD))
			{
				Status = ProcessTerrainNode(Args, XNext, ZNext + 1, NextLOD, NextLODRange, PatchCount, QPatchCount, Clip);
				if (Status != Node_Invisible)
				{
					IsVisible = true;
					if (Status == Node_NotInLOD) ChildFlags |= Child_BottomLeft;
				}
			}

			if (pCDLOD->HasNode(XNext + 1, ZNext + 1, NextLOD))
			{
				Status = ProcessTerrainNode(Args, XNext + 1, ZNext + 1, NextLOD, NextLODRange, PatchCount, QPatchCount, Clip);
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

	const CEffectConstant* pConstVSCDLODParams = NULL;
	const CEffectConstant* pConstPSCDLODParams = NULL;
	const CEffectConstant* pConstGridConsts = NULL;
	const CEffectConstant* pConstInstanceData = NULL;
	const CEffectResource* pResourceHeightMap = NULL;

	if (HMSampler.IsNullPtr()) HMSampler = GPU.CreateSampler(HMSamplerDesc);

	while (ItCurr != ItEnd)
	{
		if (ItCurr->pRenderer != this) return ItCurr;

		CTerrain* pTerrain = ItCurr->pRenderable->As<CTerrain>();

		const float VisibilityRange = 1000.f;
		const float MorphStartRatio = 0.7f;

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

		// We sort by LOD (the more is scale, the coarser is LOD), and therefore we
		// almost sort by distance to the camera, as LOD depends solely on it.
		struct CPatchInstanceCmp
		{
			inline bool operator()(const CPatchInstance& a, const CPatchInstance& b) const
			{
				return a.ScaleOffset[0] < b.ScaleOffset[0];
			}
		};

		if (PatchCount)
			std::sort(pInstances, pInstances + PatchCount, CPatchInstanceCmp());
		if (QuarterPatchCount)
			std::sort(pInstances + MaxInstanceCount - QuarterPatchCount, pInstances + MaxInstanceCount, CPatchInstanceCmp());

		// Setup lights //???here or in ProcessTerrainNode()?

		UPTR LightCount = 0;
		//...
		//Collect lights for each patch / quarterpatch, add count and indices

		// Apply material, if changed

		const CTechnique* pTech = ItCurr->pTech;
		const CPassList* pPasses = pTech->GetPasses(LightCount);
		n_assert_dbg(pPasses); // To test if it could happen at all
		if (!pPasses)
		{
			++ItCurr;
			continue;
		}

		const CMaterial* pMaterial = ItCurr->pMaterial;
		if (pMaterial != pCurrMaterial)
		{
			n_assert_dbg(pMaterial);
			n_verify_dbg(pMaterial->Apply(GPU));
			pCurrMaterial = pMaterial;

			//!!!DBG TMP!
			//Sys::DbgOut("Material changed: 0x%X\n", pMaterial);
		}

		// Pass tech params to GPU

		if (pTech != pCurrTech)
		{
			pCurrTech = pTech;

			pConstVSCDLODParams = pTech->GetConstant(CStrID("VSCDLODParams"));
			pConstPSCDLODParams = pTech->GetConstant(CStrID("PSCDLODParams"));
			pConstGridConsts = pTech->GetConstant(CStrID("GridConsts"));
			pConstInstanceData = pTech->GetConstant(CStrID("InstanceData"));
			pResourceHeightMap = pTech->GetResource(CStrID("HeightMap"));

			const CEffectSampler* pVSHeightSampler = pTech->GetSampler(CStrID("VSHeightSampler"));
			if (pVSHeightSampler)
				GPU.BindSampler(pVSHeightSampler->ShaderType, pVSHeightSampler->Handle, HMSampler.GetUnsafe());

			//!!!DBG TMP!
			//Sys::DbgOut("Tech params requested by ID\n");
		}

		if (pResourceHeightMap)
			GPU.BindResource(pResourceHeightMap->ShaderType, pResourceHeightMap->Handle, pCDLOD->GetHeightMap());

		CEffectConstSetValues PerInstanceConstValues;
		PerInstanceConstValues.SetGPU(&GPU);

		if (pConstVSCDLODParams)
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

			CDLODParams.WorldToHM[0] = 1.f / AABBSizeX;
			CDLODParams.WorldToHM[1] = 1.f / AABBSizeZ;
			CDLODParams.WorldToHM[2] = -AABBMinX * CDLODParams.WorldToHM[0];
			CDLODParams.WorldToHM[3] = -AABBMinZ * CDLODParams.WorldToHM[1];
			CDLODParams.TerrainYScale = 65535.f * pCDLOD->GetVerticalScale();
			CDLODParams.TerrainYOffset = -32767.f * pCDLOD->GetVerticalScale() + ItCurr->Transform.m[3][1]; // [3][1] = Translation.y
			CDLODParams.InvSplatSizeX = pTerrain->GetInvSplatSizeX();
			CDLODParams.InvSplatSizeZ = pTerrain->GetInvSplatSizeZ();
			CDLODParams.TexelSize[0] = 1.f / (float)pCDLOD->GetHeightMapWidth();
			CDLODParams.TexelSize[1] = 1.f / (float)pCDLOD->GetHeightMapHeight();

			PerInstanceConstValues.RegisterConstantBuffer(pConstVSCDLODParams->Desc.BufferHandle, NULL);
			PerInstanceConstValues.SetConstantValue(pConstVSCDLODParams, 0, &CDLODParams, sizeof(CDLODParams));
		
			if (pConstPSCDLODParams)
			{
				PerInstanceConstValues.RegisterConstantBuffer(pConstPSCDLODParams->Desc.BufferHandle, NULL);
				PerInstanceConstValues.SetConstantValue(pConstPSCDLODParams, 0, CDLODParams.WorldToHM, sizeof(float) * 4);
			}
		}

		if (pConstGridConsts)
			PerInstanceConstValues.RegisterConstantBuffer(pConstGridConsts->Desc.BufferHandle, NULL);

		//!!!implement looping if instance buffer is too small!
		if (pConstInstanceData)
		{
			// For D3D11 constant instancing
			NOT_IMPLEMENTED;
		}
		else
		{
			if (InstanceVB.IsNullPtr())
			{
				PVertexLayout VLInstanceData = GPU.CreateVertexLayout(InstanceDataDecl.GetPtr(), InstanceDataDecl.GetCount());
				InstanceVB = GPU.CreateVertexBuffer(*VLInstanceData, MaxInstanceCount, Access_CPU_Write | Access_GPU_Read);
			}

			void* pInstData;
			n_verify(GPU.MapResource(&pInstData, *InstanceVB, Map_WriteDiscard));
			if (PatchCount)
			{
				UPTR InstDataSize = sizeof(CPatchInstance) * PatchCount;
				memcpy(pInstData, pInstances, InstDataSize);
				pInstData = (char*)pInstData + InstDataSize;
			}
			if (QuarterPatchCount)
				memcpy(pInstData, pInstances + MaxInstanceCount - QuarterPatchCount, sizeof(CPatchInstance) * QuarterPatchCount);
			GPU.UnmapResource(*InstanceVB);
		}

		// Set vertex layout

		// In the real world we don't want to use differently laid out meshes
		n_assert_dbg(pTerrain->GetPatchMesh()->GetVertexBuffer()->GetVertexLayout() == pTerrain->GetQuarterPatchMesh()->GetVertexBuffer()->GetVertexLayout());

		const CMesh* pMesh = pTerrain->GetPatchMesh();
		n_assert_dbg(pMesh);
		CVertexBuffer* pVB = pMesh->GetVertexBuffer().GetUnsafe();
		n_assert_dbg(pVB);
		CVertexLayout* pVL = pVB->GetVertexLayout();

		//!!!implement looping if instance buffer is too small!
		if (pConstInstanceData) GPU.SetVertexLayout(pVL);
		else
		{
			IPTR VLIdx = InstancedLayouts.FindIndex(pVL);
			if (VLIdx == INVALID_INDEX)
			{
				UPTR BaseComponentCount = pVL->GetComponentCount();
				UPTR DescComponentCount = BaseComponentCount + InstanceDataDecl.GetCount();
				CVertexComponent* pInstancedDecl = (CVertexComponent*)_malloca(DescComponentCount * sizeof(CVertexComponent));
				memcpy(pInstancedDecl, pVL->GetComponent(0), BaseComponentCount * sizeof(CVertexComponent));
				memcpy(pInstancedDecl + BaseComponentCount, InstanceDataDecl.GetPtr(), InstanceDataDecl.GetCount() * sizeof(CVertexComponent));

				PVertexLayout VLInstanced = GPU.CreateVertexLayout(pInstancedDecl, DescComponentCount);

				_freea(pInstancedDecl);

				n_assert_dbg(VLInstanced.IsValidPtr());
				InstancedLayouts.Add(pVL, VLInstanced);

				GPU.SetVertexLayout(VLInstanced.GetUnsafe());
			}
			else GPU.SetVertexLayout(InstancedLayouts.ValueAt(VLIdx).GetUnsafe());
		}

		// Render patches //!!!may collect patches of different CTerrains if material is the same and instance buffer is big enough!

		//???!!!to subroutine?! to avoid duplication with quarter-patches!
		if (PatchCount)
		{
			if (pConstGridConsts)
			{
				float GridConsts[2];
				GridConsts[0] = pCDLOD->GetPatchSize() * 0.5f;
				GridConsts[1] = 1.f / GridConsts[0];
				PerInstanceConstValues.SetConstantValue(pConstGridConsts, 0, &GridConsts, sizeof(GridConsts));
			}

			PerInstanceConstValues.ApplyConstantBuffers();

			GPU.SetVertexBuffer(0, pVB);
			GPU.SetIndexBuffer(pMesh->GetIndexBuffer().GetUnsafe());

			if (!pConstInstanceData)
				GPU.SetVertexBuffer(INSTANCE_BUFFER_STREAM_INDEX, InstanceVB.GetUnsafe(), 0);

			const CPrimitiveGroup* pGroup = pMesh->GetGroup(0);
			for (UPTR PassIdx = 0; PassIdx < pPasses->GetCount(); ++PassIdx)
			{
				GPU.SetRenderState((*pPasses)[PassIdx]);
				GPU.DrawInstanced(*pGroup, PatchCount);
			}
		}

		if (QuarterPatchCount)
		{
			//!!!THIS CODE NEEDS TESTING!
			n_assert(false);

			if (pConstGridConsts)
			{
				float GridConsts[2];
				GridConsts[0] = pCDLOD->GetPatchSize() * 0.25f;
				GridConsts[1] = 1.f / GridConsts[0];
				PerInstanceConstValues.SetConstantValue(pConstGridConsts, 0, &GridConsts, sizeof(GridConsts));
			}

			PerInstanceConstValues.ApplyConstantBuffers();

			pMesh = pTerrain->GetQuarterPatchMesh();
			n_assert_dbg(pMesh);
			pVB = pMesh->GetVertexBuffer().GetUnsafe();
			n_assert_dbg(pVB);

			GPU.SetVertexBuffer(0, pVB);
			GPU.SetIndexBuffer(pMesh->GetIndexBuffer().GetUnsafe());

			if (!pConstInstanceData)
				GPU.SetVertexBuffer(INSTANCE_BUFFER_STREAM_INDEX, InstanceVB.GetUnsafe(), PatchCount);

			const CPrimitiveGroup* pGroup = pMesh->GetGroup(0);
			for (UPTR PassIdx = 0; PassIdx < pPasses->GetCount(); ++PassIdx)
			{
				GPU.SetRenderState((*pPasses)[PassIdx]);
				GPU.DrawInstanced(*pGroup, QuarterPatchCount);
			}
		}

		//!!!DBG TMP!
		//Sys::DbgOut("CTerrain rendered: %d patches, %d quarterpatches\n", PatchCount, QuarterPatchCount);

		++ItCurr;
	};

	return ItEnd;
}
//---------------------------------------------------------------------

}