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

void CTerrain::UpdatePatches(const vector3& MainCameraPos, const matrix44& ViewProjection)
{
	_Patches.clear();
	_QuarterPatches.clear();

	CAABB AABB = CDLODData->GetAABB();
	const auto LocalSize = AABB.Size();
	AABB.Transform(Transform);
	const auto WorldSize = AABB.Size();

	CNodeProcessingContext Ctx;
	Ctx.MainCameraPos = MainCameraPos;
	Ctx.ViewProjection = ViewProjection;
	Ctx.AABBMinX = AABB.Min.x;
	Ctx.AABBMinZ = AABB.Min.z;
	Ctx.ScaleBase = WorldSize / LocalSize;

	ProcessTerrainNode(Ctx, 0, 0, CDLODData->GetLODCount() - 1, EClipStatus::Clipped);

	// We sort by LOD (the higher is scale, the coarser is LOD), and therefore we almost sort by distance to the camera, as LOD depends solely on it
	std::sort(_Patches.begin(), _Patches.end(), [](const auto& a, const auto& b) { return a.ScaleOffset[0] < b.ScaleOffset[0]; });
	std::sort(_QuarterPatches.begin(), _QuarterPatches.end(), [](const auto& a, const auto& b) { return a.ScaleOffset[0] < b.ScaleOffset[0]; });
}
//---------------------------------------------------------------------

CTerrain::ENodeStatus CTerrain::ProcessTerrainNode(const CNodeProcessingContext& Ctx, U32 X, U32 Z, U32 LOD, EClipStatus Clip)
{
	I16 MinY, MaxY;
	CDLODData->GetMinMaxHeight(X, Z, LOD, MinY, MaxY);

	// Node has no data, skip it completely
	if (MaxY < MinY) return ENodeStatus::Invisible;

	const U32 NodeSize = CDLODData->GetPatchSize() << LOD;
	const float ScaleX = NodeSize * Ctx.ScaleBase.x;
	const float ScaleZ = NodeSize * Ctx.ScaleBase.z;
	const float NodeMinX = Ctx.AABBMinX + X * ScaleX;
	const float NodeMinZ = Ctx.AABBMinZ + Z * ScaleZ;

	CAABB NodeAABB;
	NodeAABB.Min.x = NodeMinX;
	NodeAABB.Min.y = MinY * Ctx.ScaleBase.y * CDLODData->GetVerticalScale();
	NodeAABB.Min.z = NodeMinZ;
	NodeAABB.Max.x = NodeMinX + ScaleX;
	NodeAABB.Max.y = MaxY * Ctx.ScaleBase.y * CDLODData->GetVerticalScale();
	NodeAABB.Max.z = NodeMinZ + ScaleZ;

	if (Clip == EClipStatus::Clipped)
	{
		// NB: Visibility is tested for the current camera, NOT always for the main
		Clip = NodeAABB.GetClipStatus(Ctx.ViewProjection);
		if (Clip == EClipStatus::Outside) return ENodeStatus::Invisible;
	}

	const auto& CurrLODParams = LODParams[LOD];

	// NB: Always must check the main frame camera, even if some special camera is used for intermediate rendering
	sphere LODSphere(Ctx.MainCameraPos, CurrLODParams.Range);
	if (LODSphere.GetClipStatus(NodeAABB) == EClipStatus::Outside) return ENodeStatus::NotInLOD;

	// Bits 0 to 3 - if set, add quarterpatch for child[0 .. 3]
	U8 ChildFlags = 0;
	enum
	{
		Child_TopLeft = 0x01,
		Child_TopRight = 0x02,
		Child_BottomLeft = 0x04,
		Child_BottomRight = 0x08,
		Child_All = (Child_TopLeft | Child_TopRight | Child_BottomLeft | Child_BottomRight)
	};

	if (LOD == 0)
	{
		// Can't subdivide any further, add the whole node
		ChildFlags = Child_All;
	}
	else
	{
		bool IsVisible = false;

		const U32 NextLOD = LOD - 1;

		// NB: Always must check the main frame camera, even if some special camera is used for intermediate rendering
		LODSphere.r = LODParams[NextLOD].Range;
		if (LODSphere.GetClipStatus(NodeAABB) == EClipStatus::Outside)
		{
			// Add the whole node to the current LOD
			ChildFlags = Child_All;
		}
		else
		{
			const U32 XNext = X << 1;
			const U32 ZNext = Z << 1;

			ENodeStatus Status = ProcessTerrainNode(Ctx, XNext, ZNext, NextLOD, Clip);
			if (Status != ENodeStatus::Invisible)
			{
				IsVisible = true;
				if (Status == ENodeStatus::NotInLOD) ChildFlags |= Child_TopLeft;
			}

			if (CDLODData->HasNode(XNext + 1, ZNext, NextLOD))
			{
				Status = ProcessTerrainNode(Ctx, XNext + 1, ZNext, NextLOD, Clip);
				if (Status != ENodeStatus::Invisible)
				{
					IsVisible = true;
					if (Status == ENodeStatus::NotInLOD) ChildFlags |= Child_TopRight;
				}
			}

			if (CDLODData->HasNode(XNext, ZNext + 1, NextLOD))
			{
				Status = ProcessTerrainNode(Ctx, XNext, ZNext + 1, NextLOD, Clip);
				if (Status != ENodeStatus::Invisible)
				{
					IsVisible = true;
					if (Status == ENodeStatus::NotInLOD) ChildFlags |= Child_BottomLeft;
				}
			}

			if (CDLODData->HasNode(XNext + 1, ZNext + 1, NextLOD))
			{
				Status = ProcessTerrainNode(Ctx, XNext + 1, ZNext + 1, NextLOD, Clip);
				if (Status != ENodeStatus::Invisible)
				{
					IsVisible = true;
					if (Status == ENodeStatus::NotInLOD) ChildFlags |= Child_BottomRight;
				}
			}
		}

		if (!ChildFlags) return IsVisible ? ENodeStatus::Processed : ENodeStatus::Invisible;
	}

	if (ChildFlags == Child_All)
	{
		// Add whole patch
		CPatchInstance Patch;
		Patch.ScaleOffset[0] = NodeAABB.Max.x - NodeAABB.Min.x;
		Patch.ScaleOffset[1] = NodeAABB.Max.z - NodeAABB.Min.z;
		Patch.ScaleOffset[2] = NodeAABB.Min.x;
		Patch.ScaleOffset[3] = NodeAABB.Min.z;
		Patch.MorphConsts[0] = CurrLODParams.Morph1;
		Patch.MorphConsts[1] = CurrLODParams.Morph2;
		_Patches.push_back(std::move(Patch));
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
			CPatchInstance Patch;
			Patch.ScaleOffset[0] = HalfScaleX;
			Patch.ScaleOffset[1] = HalfScaleZ;
			Patch.ScaleOffset[2] = NodeMinX;
			Patch.ScaleOffset[3] = NodeMinZ;
			Patch.MorphConsts[0] = CurrLODParams.Morph1;
			Patch.MorphConsts[1] = CurrLODParams.Morph2;
			_QuarterPatches.push_back(std::move(Patch));
		}

		if (ChildFlags & Child_TopRight)
		{
			CPatchInstance Patch;
			Patch.ScaleOffset[0] = HalfScaleX;
			Patch.ScaleOffset[1] = HalfScaleZ;
			Patch.ScaleOffset[2] = NodeMinX + HalfScaleX;
			Patch.ScaleOffset[3] = NodeMinZ;
			Patch.MorphConsts[0] = CurrLODParams.Morph1;
			Patch.MorphConsts[1] = CurrLODParams.Morph2;
			_QuarterPatches.push_back(std::move(Patch));
		}

		if (ChildFlags & Child_BottomLeft)
		{
			CPatchInstance Patch;
			Patch.ScaleOffset[0] = HalfScaleX;
			Patch.ScaleOffset[1] = HalfScaleZ;
			Patch.ScaleOffset[2] = NodeMinX;
			Patch.ScaleOffset[3] = NodeMinZ + HalfScaleZ;
			Patch.MorphConsts[0] = CurrLODParams.Morph1;
			Patch.MorphConsts[1] = CurrLODParams.Morph2;
			_QuarterPatches.push_back(std::move(Patch));
		}

		if (ChildFlags & Child_BottomRight)
		{
			CPatchInstance Patch;
			Patch.ScaleOffset[0] = HalfScaleX;
			Patch.ScaleOffset[1] = HalfScaleZ;
			Patch.ScaleOffset[2] = NodeMinX + HalfScaleX;
			Patch.ScaleOffset[3] = NodeMinZ + HalfScaleZ;
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
		//!!!Always must check the Main camera!
		float MaxDistToCameraSq = NodeAABB.MaxDistFromPointSq(RenderSrv->GetCameraPosition());
		float MorphStart = MorphConsts[LOD + 1].Start;
		if (MaxDistToCameraSq > MorphStart * MorphStart)
			Sys::Error("Visibility distance is too small!");
	}
#endif
	*/

	return ENodeStatus::Processed;
}
//---------------------------------------------------------------------

}
