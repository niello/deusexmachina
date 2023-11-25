#include "Terrain.h"
#include <Render/Material.h>
#include <Render/CDLODData.h>
#include <Render/Mesh.h>
#include <Render/Texture.h>
#include <Data/Algorithms.h>
#include <Core/Factory.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CTerrain, 'TERR', Render::IRenderable);

CTerrain::CTerrain() = default;
CTerrain::~CTerrain() = default;

void CTerrain::UpdateMorphConstants(float VisibilityRange)
{
	// NB: recalculate even if this value didn't change
	_VisibilityRange = VisibilityRange;

	const U32 LODCount = CDLODData ? CDLODData->GetLODCount() : 0;
	LODParams.resize(LODCount);
	LODParams.shrink_to_fit();

	float MorphStart = 0.f;
	for (U32 LOD = 0; LOD < LODCount; ++LOD)
	{
		float LODRange = VisibilityRange / static_cast<float>(1 << (LODCount - 1 - LOD));

		// Hack, see original CDLOD code. LOD 0 range is 0.9 of what is expected.
		if (!LOD) LODRange *= 0.9f;

		MorphStart = n_lerp(MorphStart, LODRange, MorphStartRatio);
		const float MorphEnd = n_lerp(LODRange, MorphStart, 0.01f);

		auto& CurrLODParams = LODParams[LOD];
		CurrLODParams.Range = LODRange;
		CurrLODParams.Morph2 = 1.0f / (MorphEnd - MorphStart);
		CurrLODParams.Morph1 = MorphEnd * CurrLODParams.Morph2;
	}
}
//---------------------------------------------------------------------

void CTerrain::UpdatePatches(const rtm::vector4f& MainCameraPos, const Math::CSIMDFrustum& ViewFrustum)
{
	constexpr TMorton RootMortonCode = 1;

	// NB: always must use the main camera for LOD selection, even if another camera (ViewFrustum) is used for intermediate rendering
	CNodeProcessingContext Ctx;
	Ctx.ViewFrustum = ViewFrustum;
	Ctx.Scale = Math::matrix_extract_scale(rtm::matrix_cast(Transform));
	Ctx.Offset = Transform.w_axis;
	Ctx.MainCameraPos = MainCameraPos;

	std::swap(_PrevPatches, _Patches);
	_Patches.clear();
	_FullPatchCount = 0;

	ProcessTerrainNode(Ctx, 0, 0, CDLODData->GetLODCount() - 1, Math::ClipIntersect, RootMortonCode);

	// We sort by LOD (the shorter is the code, the coarser is LOD), and therefore we almost sort front to back, as LOD depends solely on it
	const auto PatchInstanceCmp = [](const CPatchInstance& a, const CPatchInstance& b) { return a.MortonCode > b.MortonCode; };
	std::sort(_Patches.begin(), _Patches.end(), PatchInstanceCmp);

	// Copy cached light data where available. This is much cheaper than constructing from scratch it in CTerrainAttribute::UpdateLightList.
	// FIXME: can use one collection instead of two? Use bool IsQuarterPatch?! Pushing data to GPU is done record by record anyway!
	// Can allocate GPU CBs of necessary size and don't loop in a terrain renderer sending fixed chunks.
	//???how to apply premade CBs in a renderer alongside other per invocation shader params?!
	if (TrackObjectLightIntersections)
	{
		size_t MatchCount = 0;
		DEM::Algo::SortedInnerJoin(_Patches, _PrevPatches, PatchInstanceCmp,
			[&MatchCount, MaxLODForDynamicLights = MaxLODForDynamicLights](auto ItNew, auto ItPrev)
		{
			const bool WasLitByLOD = (ItPrev->LOD <= MaxLODForDynamicLights);
			const bool IsLitByLOD = (ItNew->LOD <= MaxLODForDynamicLights);
			if (WasLitByLOD == IsLitByLOD)
			{
				ItNew->LightsVersion = ItPrev->LightsVersion;
				ItNew->Lights = ItPrev->Lights;
				++MatchCount;
			}
		});

		// If some of new patches could not copy cached lights from prevoius ones, we must force an update of light data
		if (MatchCount < _Patches.size())
			ObjectLightIntersectionsVersion = 0;
	}
}
//---------------------------------------------------------------------

CTerrain::ENodeStatus CTerrain::ProcessTerrainNode(const CNodeProcessingContext& Ctx, TCellDim x, TCellDim z, U32 LOD, U8 ParentClipStatus, TMorton MortonCode)
{
	// Calculate node world space AABB
	rtm::vector4f NodeBoxCenter, NodeBoxExtent;
	if (!CDLODData->GetNodeAABB(x, z, LOD, NodeBoxCenter, NodeBoxExtent)) return ENodeStatus::Invisible;

	// Inside and outside statuses are propagated to children as they are, partial intersection requires further testing.
	// NB: visibility is tested for the current camera, NOT always for the main.
	if (ParentClipStatus == Math::ClipIntersect)
	{
		auto PatchBoxCenter = NodeBoxCenter;
		auto PatchBoxExtent = NodeBoxExtent;
		CDLODData->ClampNodeToPatchAABB(PatchBoxCenter, PatchBoxExtent);
		PatchBoxCenter = rtm::vector_add(PatchBoxCenter, Ctx.Offset);
		PatchBoxExtent = rtm::vector_mul(PatchBoxExtent, Ctx.Scale);
		ParentClipStatus = Math::ClipAABB(PatchBoxCenter, PatchBoxExtent, Ctx.ViewFrustum);
		if (ParentClipStatus == Math::ClipOutside) return ENodeStatus::Invisible;
	}

	NodeBoxCenter = rtm::vector_add(NodeBoxCenter, Ctx.Offset);
	NodeBoxExtent = rtm::vector_mul(NodeBoxExtent, Ctx.Scale);

	const auto LODSphere = rtm::vector_set_w(Ctx.MainCameraPos, LODParams[LOD].Range);
	if (!Math::HasIntersection(LODSphere, NodeBoxCenter, NodeBoxExtent)) return ENodeStatus::NotInLOD;

	// Bits 0 to 3 - if set, add quarterpatch for child[0 .. 3]
	U8 ChildFlags = 0;
	constexpr U8 Child_TopLeft = (1 << 0);
	constexpr U8 Child_TopRight = (1 << 1);
	constexpr U8 Child_BottomLeft = (1 << 2);
	constexpr U8 Child_BottomRight = (1 << 3);
	constexpr U8 Child_All = (Child_TopLeft | Child_TopRight | Child_BottomLeft | Child_BottomRight);

	// NB: must use correct mapping [0 .. 3] -> [LT, RT, LB, RB]
	const TMorton FirstChildMortonCode = (MortonCode << 2);

	if (LOD == DeepestLOD)
	{
		// Can't subdivide any further, add the whole node
		ChildFlags = Child_All;
	}
	else
	{
		bool IsVisible = false;

		const U32 NextLOD = LOD - 1;

		const auto NextLODSphere = rtm::vector_set_w(Ctx.MainCameraPos, LODParams[NextLOD].Range);
		if (!Math::HasIntersection(NextLODSphere, NodeBoxCenter, NodeBoxExtent))
		{
			// Add the whole node to the current LOD
			ChildFlags = Child_All;
		}
		else
		{
			const auto [HasRightChild, HasBottomChild] = CDLODData->GetChildExistence(x, z, LOD);

			const U32 NextX = x << 1;
			const U32 NextZ = z << 1;

			ENodeStatus Status = ProcessTerrainNode(Ctx, NextX, NextZ, NextLOD, ParentClipStatus, FirstChildMortonCode);
			if (Status != ENodeStatus::Invisible)
			{
				IsVisible = true;
				if (Status == ENodeStatus::NotInLOD) ChildFlags |= Child_TopLeft;
			}

			if (HasRightChild)
			{
				Status = ProcessTerrainNode(Ctx, NextX + 1, NextZ, NextLOD, ParentClipStatus, FirstChildMortonCode + 1);
				if (Status != ENodeStatus::Invisible)
				{
					IsVisible = true;
					if (Status == ENodeStatus::NotInLOD) ChildFlags |= Child_TopRight;
				}
			}

			if (HasBottomChild)
			{
				Status = ProcessTerrainNode(Ctx, NextX, NextZ + 1, NextLOD, ParentClipStatus, FirstChildMortonCode + 2);
				if (Status != ENodeStatus::Invisible)
				{
					IsVisible = true;
					if (Status == ENodeStatus::NotInLOD) ChildFlags |= Child_BottomLeft;
				}
			}

			if (HasRightChild && HasBottomChild)
			{
				Status = ProcessTerrainNode(Ctx, NextX + 1, NextZ + 1, NextLOD, ParentClipStatus, FirstChildMortonCode + 3);
				if (Status != ENodeStatus::Invisible)
				{
					IsVisible = true;
					if (Status == ENodeStatus::NotInLOD) ChildFlags |= Child_BottomRight;
				}
			}
		}

		if (!ChildFlags) return IsVisible ? ENodeStatus::Processed : ENodeStatus::Invisible;
	}

	constexpr rtm::vector4f SignMask{ 0.f, 0.f, -0.f, -0.f };

	// (HalfSizeX, HalfSizeZ, CenterX, CenterZ)
	const auto HalfSizeXZCenterXZ = Math::vector_mix_xzac(NodeBoxExtent, NodeBoxCenter);
	// (HalfSizeX, HalfSizeZ, -HalfSizeX, -HalfSizeZ)
	const auto HalfSizeXZNegHalfSizeXZ = rtm::vector_xor(Math::vector_mix_xyxy(HalfSizeXZCenterXZ), SignMask);

	// TODO: it seems to be a bit faster without full patches, using 4 quarters instead and rendering the whole terrain in 1 DIP.
	// Need to profile further on some specific cases, e.g. rendering when most of items are full. Will 4x instance increase be still negligible?
	// See https://app.asana.com/0/0/1205876475042330/f
	if (ChildFlags == Child_All)
	{
		// Add whole patch
		// (HalfSizeX + HalfSizeX, HalfSizeZ + HalfSizeZ, CenterX - HalfSizeX, CenterZ - HalfSizeZ)
		auto& Patch = _Patches.emplace_back();
		Patch.ScaleOffset = rtm::vector_add(HalfSizeXZCenterXZ, HalfSizeXZNegHalfSizeXZ);
		Patch.LOD = LOD;
		Patch.MortonCode = MortonCode;
		Patch.IsFullPatch = true;
		++_FullPatchCount;
	}
	else
	{
		// Add quarterpatches

		if (ChildFlags & Child_TopLeft)
		{
			// (HalfSizeX, HalfSizeZ, CenterX - HalfSizeX, CenterZ - HalfSizeZ)
			auto& Patch = _Patches.emplace_back();
			Patch.ScaleOffset = rtm::vector_mul_add(HalfSizeXZNegHalfSizeXZ, rtm::vector4f{ 0.f, 0.f, 1.f, 1.f }, HalfSizeXZCenterXZ);
			Patch.LOD = LOD;
			Patch.MortonCode = FirstChildMortonCode;
		}

		if (ChildFlags & Child_TopRight)
		{
			// (HalfSizeX, HalfSizeZ, CenterX, CenterZ - HalfSizeZ)
			auto& Patch = _Patches.emplace_back();
			Patch.ScaleOffset = rtm::vector_mul_add(HalfSizeXZNegHalfSizeXZ, rtm::vector4f{ 0.f, 0.f, 0.f, 1.f }, HalfSizeXZCenterXZ);
			Patch.LOD = LOD;
			Patch.MortonCode = FirstChildMortonCode + 1;
		}

		if (ChildFlags & Child_BottomLeft)
		{
			// (HalfSizeX, HalfSizeZ, CenterX - HalfSizeX, CenterZ)
			auto& Patch = _Patches.emplace_back();
			Patch.ScaleOffset = rtm::vector_mul_add(HalfSizeXZNegHalfSizeXZ, rtm::vector4f{ 0.f, 0.f, 1.f, 0.f }, HalfSizeXZCenterXZ);
			Patch.LOD = LOD;
			Patch.MortonCode = FirstChildMortonCode + 2;
		}

		if (ChildFlags & Child_BottomRight)
		{
			// (HalfSizeX, HalfSizeZ, CenterX, CenterZ)
			auto& Patch = _Patches.emplace_back();
			Patch.ScaleOffset = HalfSizeXZCenterXZ;
			Patch.LOD = LOD;
			Patch.MortonCode = FirstChildMortonCode + 3;
		}
	}

	// Morphing artifacts test (from the original CDLOD code)
	/*
#ifdef _DEBUG
	if (LOD < Terrain.LODParams.size() - 1)
	{
		const float MaxDistToCameraSq = NodeAABB.MaxDistFromPointSq(Ctx.MainCameraPos);
		const float MorphStart = MorphConsts[LOD + 1].Start;
		if (MaxDistToCameraSq > MorphStart * MorphStart)
			Sys::Error("Terrain: visibility distance is too small, morphing will be broken!");
	}
#endif
	*/

	return ENodeStatus::Processed;
}
//---------------------------------------------------------------------

}
