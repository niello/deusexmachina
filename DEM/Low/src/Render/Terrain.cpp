#include "Terrain.h"
#include <Render/Material.h>
#include <Render/CDLODData.h>
#include <Render/Mesh.h>
#include <Render/Texture.h>
#include <Math/Sphere.h>
#include <Core/Factory.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CTerrain, 'TERR', Render::IRenderable);

CTerrain::CTerrain() = default;
CTerrain::~CTerrain() = default;

void CTerrain::UpdatePatches(const vector3& MainCameraPos, const Math::CSIMDFrustum& ViewFrustum)
{
	_Patches.clear();
	_QuarterPatches.clear();

	const auto Scale = Transform.ExtractScale();
	const auto& Translation = Transform.Translation();

	// NB: always must use the main camera for LOD selection, even if another camera (ViewFrustum) is used for intermediate rendering
	CNodeProcessingContext Ctx;
	Ctx.ViewFrustum = ViewFrustum;
	Ctx.Scale = acl::vector_set(Scale.x, Scale.y, Scale.z);
	Ctx.Offset = acl::vector_set(Translation.x, Translation.y, Translation.z);
	Ctx.MainCameraPos = MainCameraPos;

	ProcessTerrainNode(Ctx, 0, 0, CDLODData->GetLODCount() - 1, Math::ClipIntersect);

	// We sort by LOD (the higher is scale, the coarser is LOD), and therefore we almost sort by distance to the camera, as LOD depends solely on it
	std::sort(_Patches.begin(), _Patches.end(), [](const auto& a, const auto& b) { return a.ScaleOffset[0] < b.ScaleOffset[0]; });
	std::sort(_QuarterPatches.begin(), _QuarterPatches.end(), [](const auto& a, const auto& b) { return a.ScaleOffset[0] < b.ScaleOffset[0]; });
}
//---------------------------------------------------------------------

CTerrain::ENodeStatus CTerrain::ProcessTerrainNode(const CNodeProcessingContext& Ctx, TCellDim x, TCellDim z, U32 LOD, U8 ParentClipStatus)
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

	const auto& CurrLODParams = LODParams[LOD];

	const auto LODSphere = acl::vector_set(Ctx.MainCameraPos.x, Ctx.MainCameraPos.y, Ctx.MainCameraPos.z, CurrLODParams.Range);
	if (!Math::HasIntersection(LODSphere, BoxCenter, BoxExtent)) return ENodeStatus::NotInLOD;

	// Bits 0 to 3 - if set, add quarterpatch for child[0 .. 3]
	U8 ChildFlags = 0;
	constexpr U8 Child_TopLeft = (1 << 0);
	constexpr U8 Child_TopRight = (1 << 1);
	constexpr U8 Child_BottomLeft = (1 << 2);
	constexpr U8 Child_BottomRight = (1 << 3);
	constexpr U8 Child_All = (Child_TopLeft | Child_TopRight | Child_BottomLeft | Child_BottomRight);

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

			const U32 NextX = X << 1;
			const U32 NextZ = Z << 1;

			ENodeStatus Status = ProcessTerrainNode(Ctx, NextX, NextZ, NextLOD, ParentClipStatus);
			if (Status != ENodeStatus::Invisible)
			{
				IsVisible = true;
				if (Status == ENodeStatus::NotInLOD) ChildFlags |= Child_TopLeft;
			}

			if (HasRightChild)
			{
				Status = ProcessTerrainNode(Ctx, NextX + 1, NextZ, NextLOD, ParentClipStatus);
				if (Status != ENodeStatus::Invisible)
				{
					IsVisible = true;
					if (Status == ENodeStatus::NotInLOD) ChildFlags |= Child_TopRight;
				}
			}

			if (HasBottomChild)
			{
				Status = ProcessTerrainNode(Ctx, NextX, NextZ + 1, NextLOD, ParentClipStatus);
				if (Status != ENodeStatus::Invisible)
				{
					IsVisible = true;
					if (Status == ENodeStatus::NotInLOD) ChildFlags |= Child_BottomLeft;
				}
			}

			if (HasRightChild && HasBottomChild)
			{
				Status = ProcessTerrainNode(Ctx, NextX + 1, NextZ + 1, NextLOD, ParentClipStatus);
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
		CPatchInstance Patch;
		Patch.ScaleOffset[0] = HalfSizeX + HalfSizeX;
		Patch.ScaleOffset[1] = HalfSizeZ + HalfSizeZ;
		Patch.ScaleOffset[2] = CenterX - HalfSizeX;
		Patch.ScaleOffset[3] = CenterZ - HalfSizeZ;
		Patch.MorphConsts[0] = CurrLODParams.Morph1;
		Patch.MorphConsts[1] = CurrLODParams.Morph2;
		_Patches.push_back(std::move(Patch));
	}
	else
	{
		// Add quarterpatches

		if (ChildFlags & Child_TopLeft)
		{
			CPatchInstance Patch;
			Patch.ScaleOffset[0] = HalfSizeX;
			Patch.ScaleOffset[1] = HalfSizeZ;
			Patch.ScaleOffset[2] = CenterX - HalfSizeX;
			Patch.ScaleOffset[3] = CenterZ - HalfSizeZ;
			Patch.MorphConsts[0] = CurrLODParams.Morph1;
			Patch.MorphConsts[1] = CurrLODParams.Morph2;
			_QuarterPatches.push_back(std::move(Patch));
		}

		if (ChildFlags & Child_TopRight)
		{
			CPatchInstance Patch;
			Patch.ScaleOffset[0] = HalfSizeX;
			Patch.ScaleOffset[1] = HalfSizeZ;
			Patch.ScaleOffset[2] = CenterX;
			Patch.ScaleOffset[3] = CenterZ - HalfSizeZ;
			Patch.MorphConsts[0] = CurrLODParams.Morph1;
			Patch.MorphConsts[1] = CurrLODParams.Morph2;
			_QuarterPatches.push_back(std::move(Patch));
		}

		if (ChildFlags & Child_BottomLeft)
		{
			CPatchInstance Patch;
			Patch.ScaleOffset[0] = HalfSizeX;
			Patch.ScaleOffset[1] = HalfSizeZ;
			Patch.ScaleOffset[2] = CenterX - HalfSizeX;
			Patch.ScaleOffset[3] = CenterZ;
			Patch.MorphConsts[0] = CurrLODParams.Morph1;
			Patch.MorphConsts[1] = CurrLODParams.Morph2;
			_QuarterPatches.push_back(std::move(Patch));
		}

		if (ChildFlags & Child_BottomRight)
		{
			CPatchInstance Patch;
			Patch.ScaleOffset[0] = HalfSizeX;
			Patch.ScaleOffset[1] = HalfSizeZ;
			Patch.ScaleOffset[2] = CenterX;
			Patch.ScaleOffset[3] = CenterZ;
			Patch.MorphConsts[0] = CurrLODParams.Morph1;
			Patch.MorphConsts[1] = CurrLODParams.Morph2;
			_QuarterPatches.push_back(std::move(Patch));
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
