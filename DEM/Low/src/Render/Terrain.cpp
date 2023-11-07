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

void CTerrain::UpdatePatches(const vector3& MainCameraPos, const Math::CSIMDFrustum& ViewFrustum)
{
	constexpr TMorton RootMortonCode = 1;

	// NB: always must use the main camera for LOD selection, even if another camera (ViewFrustum) is used for intermediate rendering
	CNodeProcessingContext Ctx;
	Ctx.ViewFrustum = ViewFrustum;
	Ctx.Scale = Math::ToSIMD(Transform.ExtractScale());
	Ctx.Offset = Math::ToSIMD(Transform.Translation());
	Ctx.MainCameraPos = MainCameraPos;

	std::swap(_PrevPatches, _Patches);
	std::swap(_PrevQuarterPatches, _QuarterPatches);
	_Patches.clear();
	_QuarterPatches.clear();

	ProcessTerrainNode(Ctx, 0, 0, CDLODData->GetLODCount() - 1, Math::ClipIntersect, RootMortonCode);

	// We sort by LOD (the shorter is the code, the coarser is LOD), and therefore we almost sort front to back, as LOD depends solely on it
	const auto PatchInstanceCmp = [](const CPatchInstance& a, const CPatchInstance& b) { return a.MortonCode > b.MortonCode; };
	std::sort(_Patches.begin(), _Patches.end(), PatchInstanceCmp);
	std::sort(_QuarterPatches.begin(), _QuarterPatches.end(), PatchInstanceCmp);

	// Copy cached light data where available. This is much cheaper than constructing from scratch it in CTerrainAttribute::UpdateLightList.
	// FIXME: can use one collection instead of two? Use bool IsQuarterPatch?! Pushing data to GPU is done record by record anyway!
	// Can allocate GPU CBs of necessary size and don't loop in a terrain renderer sending fixed chunks.
	//???how to apply premade CBs in a renderer alongside other per invocation shader params?!
	if (TrackObjectLightIntersections)
	{
		size_t MatchCount = 0;
		DEM::Algo::SortedInnerJoin(_Patches, _PrevPatches, PatchInstanceCmp, [&MatchCount](auto ItNew, auto ItPrev)
		{
			ItNew->LightsVersion = ItPrev->LightsVersion;
			ItNew->GPULightIndices = ItPrev->GPULightIndices;
			++MatchCount;
		});
		DEM::Algo::SortedInnerJoin(_Patches, _PrevQuarterPatches, PatchInstanceCmp, [&MatchCount](auto ItNew, auto ItPrev)
		{
			ItNew->LightsVersion = ItPrev->LightsVersion;
			ItNew->GPULightIndices = ItPrev->GPULightIndices;
			++MatchCount;
		});
		DEM::Algo::SortedInnerJoin(_QuarterPatches, _PrevPatches, PatchInstanceCmp, [&MatchCount](auto ItNew, auto ItPrev)
		{
			ItNew->LightsVersion = ItPrev->LightsVersion;
			ItNew->GPULightIndices = ItPrev->GPULightIndices;
			++MatchCount;
		});
		DEM::Algo::SortedInnerJoin(_QuarterPatches, _PrevQuarterPatches, PatchInstanceCmp, [&MatchCount](auto ItNew, auto ItPrev)
		{
			ItNew->LightsVersion = ItPrev->LightsVersion;
			ItNew->GPULightIndices = ItPrev->GPULightIndices;
			++MatchCount;
		});

		// If some of new patches could not copy cached lights from prevoius ones, we must force an update of light data
		if (MatchCount < _Patches.size() + _QuarterPatches.size())
			ObjectLightIntersectionsVersion = 0;
	}
}
//---------------------------------------------------------------------

CTerrain::ENodeStatus CTerrain::ProcessTerrainNode(const CNodeProcessingContext& Ctx, TCellDim x, TCellDim z, U32 LOD, U8 ParentClipStatus, TMorton MortonCode)
{
	// Calculate node world space AABB
	acl::Vector4_32 BoxCenter, BoxExtent;
	if (!CDLODData->GetNodeAABB(x, z, LOD, BoxCenter, BoxExtent)) return ENodeStatus::Invisible;
	BoxExtent = acl::vector_mul(BoxExtent, Ctx.Scale);
	BoxCenter = acl::vector_add(BoxCenter, Ctx.Offset);

	// Inside and outside statuses are propagated to children as they are, partial intersection requires further testing
	if (ParentClipStatus == Math::ClipIntersect)
	{
		// NB: Visibility is tested for the current camera, NOT always for the main
		ParentClipStatus = Math::ClipAABB(BoxCenter, BoxExtent, Ctx.ViewFrustum);
		if (ParentClipStatus == Math::ClipOutside) return ENodeStatus::Invisible;
	}

	const auto LODSphere = acl::vector_set(Ctx.MainCameraPos.x, Ctx.MainCameraPos.y, Ctx.MainCameraPos.z, LODParams[LOD].Range);
	if (!Math::HasIntersection(LODSphere, BoxCenter, BoxExtent)) return ENodeStatus::NotInLOD;

	// Bits 0 to 3 - if set, add quarterpatch for child[0 .. 3]
	U8 ChildFlags = 0;
	constexpr U8 Child_TopLeft = (1 << 0);
	constexpr U8 Child_TopRight = (1 << 1);
	constexpr U8 Child_BottomLeft = (1 << 2);
	constexpr U8 Child_BottomRight = (1 << 3);
	constexpr U8 Child_All = (Child_TopLeft | Child_TopRight | Child_BottomLeft | Child_BottomRight);

	// NB: must use correct mapping [0 .. 3] -> [LT, RT, LB, RB]
	const TMorton FirstChildMortonCode = (MortonCode << 2);

	if (LOD == 0)
	{
		// Can't subdivide any further, add the whole node
		ChildFlags = Child_All;
	}
	else
	{
		bool IsVisible = false;

		const U32 NextLOD = LOD - 1;

		const auto NextLODSphere = acl::vector_set(Ctx.MainCameraPos.x, Ctx.MainCameraPos.y, Ctx.MainCameraPos.z, LODParams[NextLOD].Range);
		if (!Math::HasIntersection(NextLODSphere, BoxCenter, BoxExtent))
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

	const float CenterX = acl::vector_get_x(BoxCenter);
	const float CenterZ = acl::vector_get_z(BoxCenter);
	const float HalfSizeX = acl::vector_get_x(BoxExtent);
	const float HalfSizeZ = acl::vector_get_z(BoxExtent);

	if (ChildFlags == Child_All)
	{
		// Add whole patch
		auto& Patch = _Patches.emplace_back();
		Patch.ScaleOffset = acl::vector_set(HalfSizeX + HalfSizeX, HalfSizeZ + HalfSizeZ, CenterX - HalfSizeX, CenterZ - HalfSizeZ);
		Patch.LOD = LOD;
		Patch.MortonCode = MortonCode;
	}
	else
	{
		// Add quarterpatches

		if (ChildFlags & Child_TopLeft)
		{
			auto& Patch = _QuarterPatches.emplace_back();
			Patch.ScaleOffset = acl::vector_set(HalfSizeX, HalfSizeZ, CenterX - HalfSizeX, CenterZ - HalfSizeZ);
			Patch.LOD = LOD;
			Patch.MortonCode = FirstChildMortonCode;
		}

		if (ChildFlags & Child_TopRight)
		{
			auto& Patch = _QuarterPatches.emplace_back();
			Patch.ScaleOffset = acl::vector_set(HalfSizeX, HalfSizeZ, CenterX, CenterZ - HalfSizeZ);
			Patch.LOD = LOD;
			Patch.MortonCode = FirstChildMortonCode + 1;
		}

		if (ChildFlags & Child_BottomLeft)
		{
			auto& Patch = _QuarterPatches.emplace_back();
			Patch.ScaleOffset = acl::vector_set(HalfSizeX, HalfSizeZ, CenterX - HalfSizeX, CenterZ);
			Patch.LOD = LOD;
			Patch.MortonCode = FirstChildMortonCode + 2;
		}

		if (ChildFlags & Child_BottomRight)
		{
			auto& Patch = _QuarterPatches.emplace_back();
			Patch.ScaleOffset = acl::vector_set(HalfSizeX, HalfSizeZ, CenterX, CenterZ);
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
