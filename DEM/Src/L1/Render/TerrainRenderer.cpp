#include "TerrainRenderer.h"

#include <Render/RenderNode.h>
#include <Render/GPUDriver.h>
#include <Render/Terrain.h>
#include <Render/Light.h>
#include <Render/Material.h>
#include <Render/Effect.h>
#include <Render/ShaderConstant.h>
#include <Render/ConstantBufferSet.h>
#include <Math/Sphere.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CTerrainRenderer, 'TRNR', Render::IRenderer);

CTerrainRenderer::~CTerrainRenderer()
{
	if (pInstances) n_free_aligned(pInstances);
}
//---------------------------------------------------------------------

bool CTerrainRenderer::Init(bool LightingEnabled)
{
	// Setup dynamic enumeration
	InputSet_CDLOD = RegisterShaderInputSetID(CStrID("CDLOD"));

	HMSamplerDesc.SetDefaults();
	HMSamplerDesc.AddressU = TexAddr_Clamp;
	HMSamplerDesc.AddressV = TexAddr_Clamp;
	HMSamplerDesc.Filter = TexFilter_MinMag_Linear_Mip_Point;

	//!!!DBG TMP! //???where to define?
	MaxInstanceCount = 128;

	if (LightingEnabled)
	{
		// With stream instancing we add light indices to a vertex declaration, maximum light count is
		// determined by DEM_LIGHT_COUNT of a tech, lighting stops at the first light index == -1.
		// Clamp to maximum free VS input/output register count of 10, other 6 are used for other values.
		const UPTR LightIdxVectorCount = (INSTANCE_MAX_LIGHT_COUNT + 3) / 4;
		InstanceDataDecl.SetSize(n_min(2 + LightIdxVectorCount, 10));
	}
	else
	{
		// Only vertex shader data will be added
		InstanceDataDecl.SetSize(2);
	}

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

	// Light indices
	UPTR ElementIdx = 2;
	while (ElementIdx < InstanceDataDecl.GetCount())
	{
		pCmp = &InstanceDataDecl[ElementIdx];
		pCmp->Semantic = VCSem_TexCoord;
		pCmp->UserDefinedName = NULL;
		pCmp->Index = ElementIdx;
		pCmp->Format = VCFmt_SInt16_4;
		pCmp->Stream = INSTANCE_BUFFER_STREAM_INDEX;
		pCmp->OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
		pCmp->PerInstanceData = true;

		++ElementIdx;
	}

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
	float ScaleX = NodeSize * Args.ScaleBaseX;
	float ScaleZ = NodeSize * Args.ScaleBaseZ;
	float NodeMinX = Args.AABBMinX + X * ScaleX;
	float NodeMinZ = Args.AABBMinZ + Z * ScaleZ;

	CAABB NodeAABB;
	NodeAABB.Min.x = NodeMinX;
	NodeAABB.Min.y = MinY * Args.pCDLOD->GetVerticalScale();
	NodeAABB.Min.z = NodeMinZ;
	NodeAABB.Max.x = NodeMinX + ScaleX;
	NodeAABB.Max.y = MaxY * Args.pCDLOD->GetVerticalScale();
	NodeAABB.Max.z = NodeMinZ + ScaleZ;

	if (Sphere.GetClipStatus(NodeAABB) == Outside) FAIL;

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
	float ScaleX = NodeSize * Args.ScaleBaseX;
	float ScaleZ = NodeSize * Args.ScaleBaseZ;
	float NodeMinX = Args.AABBMinX + X * ScaleX;
	float NodeMinZ = Args.AABBMinZ + Z * ScaleZ;

	CAABB NodeAABB;
	NodeAABB.Min.x = NodeMinX;
	NodeAABB.Min.y = MinY * Args.pCDLOD->GetVerticalScale();
	NodeAABB.Min.z = NodeMinZ;
	NodeAABB.Max.x = NodeMinX + ScaleX;
	NodeAABB.Max.y = MaxY * Args.pCDLOD->GetVerticalScale();
	NodeAABB.Max.z = NodeMinZ + ScaleZ;

	if (NodeAABB.GetClipStatus(Frustum) == Outside) FAIL;

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

	U8 LightCount = 0;

	// Collect all lights that potentially touch the terrain, no limit here, limit is
	// applied to the light count of a terrain patch instance.
	if (Context.pLights)
	{
		n_assert_dbg(Context.pLightIndices);

		const CCDLODData& CDLOD = *pTerrain->GetCDLODData();
		const U32 TopPatchesW = CDLOD.GetTopPatchCountW();
		const U32 TopPatchesH = CDLOD.GetTopPatchCountH();
		const U32 TopLOD = CDLOD.GetLODCount() - 1;

		float AABBMinX = Context.AABB.Min.x;
		float AABBMinZ = Context.AABB.Min.z;
		float AABBSizeX = Context.AABB.Max.x - AABBMinX;
		float AABBSizeZ = Context.AABB.Max.z - AABBMinZ;

		CLightTestArgs Args;
		Args.pCDLOD = pTerrain->GetCDLODData();
		Args.AABBMinX = AABBMinX;
		Args.AABBMinZ = AABBMinZ;
		Args.ScaleBaseX = AABBSizeX / (float)(pTerrain->GetCDLODData()->GetHeightMapWidth() - 1);
		Args.ScaleBaseZ = AABBSizeZ / (float)(pTerrain->GetCDLODData()->GetHeightMapHeight() - 1);

		CArray<U16>& LightIndices = *Context.pLightIndices;
		Node.LightIndexBase = LightIndices.GetCount();

		const CArray<CLightRecord>& Lights = *Context.pLights;
		for (UPTR i = 0; i < Lights.GetCount(); ++i)
		{
			CLightRecord& LightRec = Lights[i];
			const CLight* pLight = LightRec.pLight;

			switch (pLight->Type)
			{
				case Light_Point:
				{
					//!!!???avoid object creation, rewrite functions so that testing against vector + float is possible!?
					sphere LightBounds(LightRec.Transform.Translation(), pLight->GetRange());
					if (LightBounds.GetClipStatus(Context.AABB) == Outside) continue;

					// Perform coarse test on quadtree
					if (LIGHT_INTERSECTION_COARSE_TEST_MAX_AABBS > 1)
					{
						bool Intersects = false;
						UPTR AABBTestCounter = 1;
						for (U32 Z = 0; Z < TopPatchesH; ++Z)
						{
							for (U32 X = 0; X < TopPatchesW; ++X)
								if (CheckNodeSphereIntersection(Args, LightBounds, X, Z, TopLOD, AABBTestCounter))
								{
									Intersects = true;
									break;
								}

							if (Intersects) break;
						}

						if (!Intersects) continue;
					}

					break;
				}
				case Light_Spot:
				{
					//!!!???PERF: test against sphere before?!
					//???cache GlobalFrustum in a light record?
					matrix44 LocalFrustum;
					pLight->CalcLocalFrustum(LocalFrustum);
					matrix44 GlobalFrustum;
					LightRec.Transform.invert_simple(GlobalFrustum);
					GlobalFrustum *= LocalFrustum;
					if (Context.AABB.GetClipStatus(GlobalFrustum) == Outside) continue;

					// Perform coarse test on quadtree
					if (LIGHT_INTERSECTION_COARSE_TEST_MAX_AABBS > 1)
					{
						bool Intersects = false;
						UPTR AABBTestCounter = 1;
						for (U32 Z = 0; Z < TopPatchesH; ++Z)
						{
							for (U32 X = 0; X < TopPatchesW; ++X)
								if (CheckNodeFrustumIntersection(Args, GlobalFrustum, X, Z, TopLOD, AABBTestCounter))
								{
									Intersects = true;
									break;
								}

							if (Intersects) break;
						}

						if (!Intersects) continue;
					}

					break;
				}
			}

			LightIndices.Add(i);
			++LightCount;
			++LightRec.UseCount;
		}
	}

	Node.LightCount = LightCount;

	OK;
}
//---------------------------------------------------------------------

void CTerrainRenderer::FillNodeLightIndices(const CProcessTerrainNodeArgs& Args, CPatchInstance& Patch, const CAABB& NodeAABB, U8& MaxLightCount)
{
	n_assert_dbg(Args.pRenderContext->pLightIndices);

	const CArray<U16>& LightIndices = *Args.pRenderContext->pLightIndices;

	UPTR LightCount = 0;

	const CArray<CLightRecord>& Lights = *Args.pRenderContext->pLights;
	for (UPTR i = 0; i < Args.LightCount; ++i)
	{
		CLightRecord& LightRec = Lights[i];
		const CLight* pLight = LightRec.pLight;
		switch (pLight->Type)
		{
			case Light_Point:
			{
				//!!!???avoid object creation, rewrite functions so that testing against vector + float is possible!?
				sphere LightBounds(LightRec.Transform.Translation(), pLight->GetRange());
				if (LightBounds.GetClipStatus(NodeAABB) == Outside) continue;
				break;
			}
			case Light_Spot:
			{
				//!!!???PERF: test against sphere before?!
				//???cache GlobalFrustum in a light record?
				matrix44 LocalFrustum;
				pLight->CalcLocalFrustum(LocalFrustum);
				matrix44 GlobalFrustum;
				LightRec.Transform.invert_simple(GlobalFrustum);
				GlobalFrustum *= LocalFrustum;
				if (NodeAABB.GetClipStatus(GlobalFrustum) == Outside) continue;
				break;
			}
		}

		// Don't want to calculate priorities, just skip all remaining lights (for simplicity, may rework later)
		if (LightCount >= INSTANCE_MAX_LIGHT_COUNT) break;

		Patch.LightIndex[LightCount] = LightRec.GPULightIndex;
		++LightCount;
	}

	if (LightCount < INSTANCE_MAX_LIGHT_COUNT)
		Patch.LightIndex[LightCount] = -1;

	if (MaxLightCount < LightCount)
		MaxLightCount = LightCount;
}
//---------------------------------------------------------------------

//!!!recalculation required only on viewer's position / look vector change!
CTerrainRenderer::ENodeStatus CTerrainRenderer::ProcessTerrainNode(const CProcessTerrainNodeArgs& Args,
																   U32 X, U32 Z, U32 LOD, float LODRange,
																   U32& PatchCount, U32& QPatchCount,
																   U8& MaxLightCount, EClipStatus Clip)
{
	const CCDLODData* pCDLOD = Args.pCDLOD;

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
		Clip = NodeAABB.GetClipStatus(Args.pRenderContext->ViewProjection);
		if (Clip == Outside) return Node_Invisible;
	}

	// NB: Always must check the main frame camera, even if some special camera is used for intermediate rendering
	sphere LODSphere(Args.pRenderContext->CameraPosition, LODRange);
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
		LODSphere.r = NextLODRange;
		if (LODSphere.GetClipStatus(NodeAABB) == Outside)
		{
			// Add the whole node to the current LOD
			ChildFlags = Child_All;
		}
		else
		{
			const U32 XNext = X << 1, ZNext = Z << 1, NextLOD = LOD - 1;

			ENodeStatus Status = ProcessTerrainNode(Args, XNext, ZNext, NextLOD, NextLODRange, PatchCount, QPatchCount, MaxLightCount, Clip);
			if (Status != Node_Invisible)
			{
				IsVisible = true;
				if (Status == Node_NotInLOD) ChildFlags |= Child_TopLeft;
			}

			if (pCDLOD->HasNode(XNext + 1, ZNext, NextLOD))
			{
				Status = ProcessTerrainNode(Args, XNext + 1, ZNext, NextLOD, NextLODRange, PatchCount, QPatchCount, MaxLightCount, Clip);
				if (Status != Node_Invisible)
				{
					IsVisible = true;
					if (Status == Node_NotInLOD) ChildFlags |= Child_TopRight;
				}
			}

			if (pCDLOD->HasNode(XNext, ZNext + 1, NextLOD))
			{
				Status = ProcessTerrainNode(Args, XNext, ZNext + 1, NextLOD, NextLODRange, PatchCount, QPatchCount, MaxLightCount, Clip);
				if (Status != Node_Invisible)
				{
					IsVisible = true;
					if (Status == Node_NotInLOD) ChildFlags |= Child_BottomLeft;
				}
			}

			if (pCDLOD->HasNode(XNext + 1, ZNext + 1, NextLOD))
			{
				Status = ProcessTerrainNode(Args, XNext + 1, ZNext + 1, NextLOD, NextLODRange, PatchCount, QPatchCount, MaxLightCount, Clip);
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

	const bool LightingEnabled = (Args.pRenderContext->pLights != NULL);

	float* pLODMorphConsts = Args.pMorphConsts + 2 * LOD;

	if (ChildFlags == Child_All)
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

		if (LightingEnabled && INSTANCE_MAX_LIGHT_COUNT)
			FillNodeLightIndices(Args, Patch, NodeAABB, MaxLightCount);

		++PatchCount;
	}
	else
	{
		// Add quarterpatches

		float NodeMinX = NodeAABB.Min.x;
		float NodeMinZ = NodeAABB.Min.z;
		float ScaleX = (NodeAABB.Max.x - NodeAABB.Min.x);
		float ScaleZ = (NodeAABB.Max.z - NodeAABB.Min.z);
		float HalfScaleX = ScaleX * 0.5f;
		float HalfScaleZ = ScaleZ * 0.5f;

		// For lighting. We don't request minmax Y for quarterpatches, but we could.
		CAABB QuarterNodeAABB;
		QuarterNodeAABB.Min.y = NodeAABB.Min.y;
		QuarterNodeAABB.Max.y = NodeAABB.Max.y;

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

			if (LightingEnabled && INSTANCE_MAX_LIGHT_COUNT)
			{
				QuarterNodeAABB.Min.x = NodeMinX;
				QuarterNodeAABB.Min.z = NodeMinZ;
				QuarterNodeAABB.Max.x = NodeMinX + HalfScaleX;
				QuarterNodeAABB.Max.z = NodeMinZ + HalfScaleZ;
				FillNodeLightIndices(Args, Patch, QuarterNodeAABB, MaxLightCount);
			}

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

			if (LightingEnabled && INSTANCE_MAX_LIGHT_COUNT)
			{
				QuarterNodeAABB.Min.x = NodeMinX + HalfScaleX;
				QuarterNodeAABB.Min.z = NodeMinZ;
				QuarterNodeAABB.Max.x = NodeMinX + ScaleX;
				QuarterNodeAABB.Max.z = NodeMinZ + HalfScaleZ;
				FillNodeLightIndices(Args, Patch, QuarterNodeAABB, MaxLightCount);
			}

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

			if (LightingEnabled && INSTANCE_MAX_LIGHT_COUNT)
			{
				QuarterNodeAABB.Min.x = NodeMinX;
				QuarterNodeAABB.Min.z = NodeMinZ + HalfScaleZ;
				QuarterNodeAABB.Max.x = NodeMinX + HalfScaleX;
				QuarterNodeAABB.Max.z = NodeMinZ + ScaleZ;
				FillNodeLightIndices(Args, Patch, QuarterNodeAABB, MaxLightCount);
			}

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

			if (LightingEnabled && INSTANCE_MAX_LIGHT_COUNT)
			{
				QuarterNodeAABB.Min.x = NodeMinX + HalfScaleX;
				QuarterNodeAABB.Min.z = NodeMinZ + HalfScaleZ;
				QuarterNodeAABB.Max.x = NodeMinX + ScaleX;
				QuarterNodeAABB.Max.z = NodeMinZ + ScaleZ;
				FillNodeLightIndices(Args, Patch, QuarterNodeAABB, MaxLightCount);
			}

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

CArray<CRenderNode*>::CIterator CTerrainRenderer::Render(const CRenderContext& Context, CArray<CRenderNode*>& RenderQueue, CArray<CRenderNode*>::CIterator ItCurr)
{
	CGPUDriver& GPU = *Context.pGPU;

	CArray<CRenderNode*>::CIterator ItEnd = RenderQueue.End();

	if (!GPU.CheckCaps(Caps_VSTexFiltering_Linear))
	{
		// Skip terrain rendering. Can fall back to manual 4-sample filtering in a shader instead.
		while (ItCurr != ItEnd)
		{
			if ((*ItCurr)->pRenderer != this) return ItCurr;
			++ItCurr;
		}
		return ItEnd;
	}

	const CMaterial* pCurrMaterial = NULL;
	const CTechnique* pCurrTech = NULL;

	const CEffectConstant* pConstVSCDLODParams = NULL;
	const CEffectConstant* pConstPSCDLODParams = NULL;
	const CEffectConstant* pConstGridConsts = NULL;
	const CEffectConstant* pConstInstanceDataVS = NULL;
	const CEffectConstant* pConstInstanceDataPS = NULL;
	const CEffectResource* pResourceHeightMap = NULL;

	const bool LightingEnabled = (Context.pLights != NULL);

	if (HMSampler.IsNullPtr()) HMSampler = GPU.CreateSampler(HMSamplerDesc);

	while (ItCurr != ItEnd)
	{
		CRenderNode* pRenderNode = *ItCurr;

		if (pRenderNode->pRenderer != this) return ItCurr;

		CTerrain* pTerrain = pRenderNode->pRenderable->As<CTerrain>();

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
		AABB.Transform(pRenderNode->Transform);
		float AABBMinX = AABB.Min.x;
		float AABBMinZ = AABB.Min.z;
		float AABBSizeX = AABB.Max.x - AABBMinX;
		float AABBSizeZ = AABB.Max.z - AABBMinZ;

		CProcessTerrainNodeArgs Args;
		Args.pCDLOD = pTerrain->GetCDLODData();
		Args.pInstances = pInstances;
		Args.pMorphConsts = pMorphConsts;
		Args.pRenderContext = &Context;
		Args.MaxInstanceCount = MaxInstanceCount;
		Args.AABBMinX = AABBMinX;
		Args.AABBMinZ = AABBMinZ;
		Args.ScaleBaseX = AABBSizeX / (float)(pCDLOD->GetHeightMapWidth() - 1);
		Args.ScaleBaseZ = AABBSizeZ / (float)(pCDLOD->GetHeightMapHeight() - 1);
		Args.LightIndexBase = pRenderNode->LightIndexBase;
		Args.LightCount = pRenderNode->LightCount;

		U8 MaxLightCount = 0;
		const U32 TopPatchesW = pCDLOD->GetTopPatchCountW();
		const U32 TopPatchesH = pCDLOD->GetTopPatchCountH();
		const U32 TopLOD = LODCount - 1;
		for (U32 Z = 0; Z < TopPatchesH; ++Z)
			for (U32 X = 0; X < TopPatchesW; ++X)
				ProcessTerrainNode(Args, X, Z, TopLOD, VisibilityRange, PatchCount, QuarterPatchCount, MaxLightCount);

		// Since we allocate instance stream based on maximum light count, we never reallocate it when frame max light count
		// drops compared to the previous frame, to avoid per-frame VB recreation. Instead we remember the biggest light
		// count ever requested, allocate VB for it, and live without VB reallocations until more lights are required.
		// The first unused light index is always equal -1 so no matter how much lights are, all unused ones are ignored.
		// For constant instancing this value never gets used, tech with LightCount = 0 is always selected.
		if (CurrMaxLightCount > MaxLightCount) MaxLightCount = CurrMaxLightCount;

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

		// Select tech for the maximal light count used per-patch

		UPTR LightCount = MaxLightCount;
		const CTechnique* pTech = pRenderNode->pTech;
		const CPassList* pPasses = pTech->GetPasses(LightCount);
		n_assert_dbg(pPasses); // To test if it could happen at all
		if (!pPasses)
		{
			++ItCurr;
			continue;
		}

		if (LightingEnabled)
		{
			if (CurrMaxLightCount < LightCount) CurrMaxLightCount = LightCount;

			if (!Context.UsesGlobalLightBuffer)
			{
				NOT_IMPLEMENTED;
			}
		}

		// Apply material, if changed

		const CMaterial* pMaterial = pRenderNode->pMaterial;
		if (pMaterial != pCurrMaterial)
		{
			n_assert_dbg(pMaterial);
			n_verify_dbg(pMaterial->Apply(GPU));
			pCurrMaterial = pMaterial;
		}

		// Pass tech params to GPU

		if (pTech != pCurrTech)
		{
			pCurrTech = pTech;

			pConstVSCDLODParams = pTech->GetConstant(CStrID("VSCDLODParams"));
			pConstPSCDLODParams = pTech->GetConstant(CStrID("PSCDLODParams"));
			pConstGridConsts = pTech->GetConstant(CStrID("GridConsts"));
			pConstInstanceDataVS = pTech->GetConstant(CStrID("InstanceDataVS"));
			pConstInstanceDataPS = pTech->GetConstant(CStrID("InstanceDataPS"));
			pResourceHeightMap = pTech->GetResource(CStrID("HeightMapVS"));
			//???normal map?! now in material, must not be there!

			const CEffectSampler* pVSLinearSampler = pTech->GetSampler(CStrID("VSLinearSampler"));
			if (pVSLinearSampler)
				GPU.BindSampler(pVSLinearSampler->ShaderType, pVSLinearSampler->Handle, HMSampler.GetUnsafe());
		}

		if (pResourceHeightMap)
			GPU.BindResource(pResourceHeightMap->ShaderType, pResourceHeightMap->Handle, pCDLOD->GetHeightMap());

		CConstantBufferSet PerInstanceBuffers;
		PerInstanceBuffers.SetGPU(&GPU);

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
			CDLODParams.TerrainYOffset = -32767.f * pCDLOD->GetVerticalScale() + pRenderNode->Transform.m[3][1]; // [3][1] = Translation.y
			CDLODParams.InvSplatSizeX = pTerrain->GetInvSplatSizeX();
			CDLODParams.InvSplatSizeZ = pTerrain->GetInvSplatSizeZ();
			CDLODParams.TexelSize[0] = 1.f / (float)pCDLOD->GetHeightMapWidth();
			CDLODParams.TexelSize[1] = 1.f / (float)pCDLOD->GetHeightMapHeight();

			CConstantBuffer* pCB = PerInstanceBuffers.RequestBuffer(pConstVSCDLODParams->Const->GetConstantBufferHandle(), pConstVSCDLODParams->ShaderType);
			pConstVSCDLODParams->Const->SetRawValue(*pCB, &CDLODParams, sizeof(CDLODParams));
		
			if (pConstPSCDLODParams)
			{
				CConstantBuffer* pCB = PerInstanceBuffers.RequestBuffer(pConstPSCDLODParams->Const->GetConstantBufferHandle(), pConstPSCDLODParams->ShaderType);
				pConstPSCDLODParams->Const->SetRawValue(*pCB, CDLODParams.WorldToHM, sizeof(float) * 4);
			}
		}

		//!!!implement looping if instance buffer is too small!
		if (pConstInstanceDataVS)
		{
			// For D3D11 constant instancing
			NOT_IMPLEMENTED;
		}
		else
		{
			// Calculate int4 light index vector count supported by both tech and vertex declaration.
			// Vertex declaration size will be 2 elements for VS data + one element per vector.
			const UPTR LightVectorCount = (LightCount + 3) / 4;
			const UPTR DeclSize = 2 + LightVectorCount;
			n_assert_dbg(DeclSize <= InstanceDataDecl.GetCount());

			// If current buffer doesn't suit the tech, recreate it. Since we only grow tech LightCount and never
			// shrink it, buffer reallocation will be requested only if can't handle desired light count.
			if (InstanceVB.IsNullPtr() || InstanceVB->GetVertexLayout()->GetComponentCount() != DeclSize)
			{
				PVertexLayout VLInstanceData = GPU.CreateVertexLayout(InstanceDataDecl.GetPtr(), DeclSize);
				InstanceVB = NULL; // Drop before allocating new buffer
				InstanceVB = GPU.CreateVertexBuffer(*VLInstanceData, MaxInstanceCount, Access_CPU_Write | Access_GPU_Read);

				//!!!DBG TMP!
				Sys::DbgOut("New terrain instance buffer allocated, %d vertex decl elements\n", DeclSize);
			}

			// Upload instance data to IA stream

			//???what about D3D11_APPEND_ALIGNED_ELEMENT in D3D11 and float2?
			const UPTR InstanceElementSize = InstanceVB->GetVertexLayout()->GetVertexSizeInBytes();

			void* pInstData;
			n_verify(GPU.MapResource(&pInstData, *InstanceVB, Map_WriteDiscard));
			
			if (PatchCount)
			{
				UPTR InstDataSize = InstanceElementSize * PatchCount;
				if (sizeof(CPatchInstance) == InstanceElementSize)
				{
					memcpy(pInstData, pInstances, InstDataSize);
				}
				else
				{
					char* pCurrInstData = (char*)pInstData;
					for (UPTR PatchIdx = 0; PatchIdx < PatchCount; ++PatchIdx)
					{
						memcpy(pCurrInstData, pInstances + PatchIdx, InstanceElementSize);
						pCurrInstData += InstanceElementSize;
					}
				}
				pInstData = (char*)pInstData + InstDataSize;
			}
			
			if (QuarterPatchCount)
			{
				const CPatchInstance* pQInstances = pInstances + MaxInstanceCount - QuarterPatchCount;
				if (sizeof(CPatchInstance) == InstanceElementSize)
				{
					memcpy(pInstData, pQInstances, InstanceElementSize * QuarterPatchCount);
				}
				else
				{
					char* pCurrInstData = (char*)pInstData;
					for (UPTR PatchIdx = 0; PatchIdx < QuarterPatchCount; ++PatchIdx)
					{
						memcpy(pCurrInstData, pQInstances + PatchIdx, InstanceElementSize);
						pCurrInstData += InstanceElementSize;
					}
				}
			}

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
		if (pConstInstanceDataVS) GPU.SetVertexLayout(pVL);
		else
		{
			IPTR VLIdx = InstancedLayouts.FindIndex(pVL);
			if (VLIdx == INVALID_INDEX)
			{
				UPTR BaseComponentCount = pVL->GetComponentCount();
				UPTR InstComponentCount = InstanceVB->GetVertexLayout()->GetComponentCount();
				UPTR DescComponentCount = BaseComponentCount + InstComponentCount;
				CVertexComponent* pInstancedDecl = (CVertexComponent*)_malloca(DescComponentCount * sizeof(CVertexComponent));
				memcpy(pInstancedDecl, pVL->GetComponent(0), BaseComponentCount * sizeof(CVertexComponent));
				memcpy(pInstancedDecl + BaseComponentCount, InstanceDataDecl.GetPtr(), InstComponentCount * sizeof(CVertexComponent));

				PVertexLayout VLInstanced = GPU.CreateVertexLayout(pInstancedDecl, DescComponentCount);

				_freea(pInstancedDecl);

				n_assert_dbg(VLInstanced.IsValidPtr());
				InstancedLayouts.Add(pVL, VLInstanced);

				GPU.SetVertexLayout(VLInstanced.GetUnsafe());
			}
			else GPU.SetVertexLayout(InstancedLayouts.ValueAt(VLIdx).GetUnsafe());
		}

		// Render patches //!!!may collect patches of different CTerrains if material is the same and instance buffer is big enough!

		if (PatchCount)
		{
			if (pConstGridConsts)
			{
				float GridConsts[2];
				GridConsts[0] = pCDLOD->GetPatchSize() * 0.5f;
				GridConsts[1] = 1.f / GridConsts[0];
				CConstantBuffer* pCB = PerInstanceBuffers.RequestBuffer(pConstGridConsts->Const->GetConstantBufferHandle(), pConstGridConsts->ShaderType);
				pConstGridConsts->Const->SetRawValue(*pCB, &GridConsts, sizeof(GridConsts));
			}

			PerInstanceBuffers.CommitChanges();

			GPU.SetVertexBuffer(0, pVB);
			GPU.SetIndexBuffer(pMesh->GetIndexBuffer().GetUnsafe());

			if (!pConstInstanceDataVS)
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
			if (pConstGridConsts)
			{
				float GridConsts[2];
				GridConsts[0] = pCDLOD->GetPatchSize() * 0.25f;
				GridConsts[1] = 1.f / GridConsts[0];
				CConstantBuffer* pCB = PerInstanceBuffers.RequestBuffer(pConstGridConsts->Const->GetConstantBufferHandle(), pConstGridConsts->ShaderType);
				pConstGridConsts->Const->SetRawValue(*pCB, &GridConsts, sizeof(GridConsts));
			}

			PerInstanceBuffers.CommitChanges();

			pMesh = pTerrain->GetQuarterPatchMesh();
			n_assert_dbg(pMesh);
			pVB = pMesh->GetVertexBuffer().GetUnsafe();
			n_assert_dbg(pVB);

			GPU.SetVertexBuffer(0, pVB);
			GPU.SetIndexBuffer(pMesh->GetIndexBuffer().GetUnsafe());

			if (!pConstInstanceDataVS)
				GPU.SetVertexBuffer(INSTANCE_BUFFER_STREAM_INDEX, InstanceVB.GetUnsafe(), PatchCount);

			const CPrimitiveGroup* pGroup = pMesh->GetGroup(0);
			for (UPTR PassIdx = 0; PassIdx < pPasses->GetCount(); ++PassIdx)
			{
				GPU.SetRenderState((*pPasses)[PassIdx]);
				GPU.DrawInstanced(*pGroup, QuarterPatchCount);
			}
		}

		++ItCurr;
	};

	return ItEnd;
}
//---------------------------------------------------------------------

}