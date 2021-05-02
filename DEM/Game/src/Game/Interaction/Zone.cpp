#include "Zone.h"
#include <DetourCommon.h>

namespace DEM::Game
{

// NB: if closed, must be convex
static float SqDistanceToPolyChain(const vector3& Pos, const float* pVertices, UPTR VertexCount, bool Closed, UPTR& OutSegment, float& OutT)
{
	float MinSqDistance = std::numeric_limits<float>().max();
	if (Closed)
	{
		// NB: only convex polys are supported for now!
		if (dtPointInPolygon(Pos.v, pVertices, VertexCount))
		{
			OutSegment = VertexCount;
			return 0.f;
		}

		// Process implicit closing edge
		MinSqDistance = dtDistancePtSegSqr2D(Pos.v, &pVertices[3 * (VertexCount - 1)], &pVertices[0], OutT);
		OutSegment = VertexCount - 1;
	}

	for (UPTR i = 0; i < VertexCount - 1; ++i)
	{
		float t;
		const float SqDistance = dtDistancePtSegSqr2D(Pos.v, &pVertices[3 * i], &pVertices[3 * (i + 1)], t);
		if (SqDistance < MinSqDistance)
		{
			MinSqDistance = SqDistance;
			OutSegment = i;
			OutT = t;
		}
	}

	return MinSqDistance;
}
//---------------------------------------------------------------------

// Returns a distance from the closest point to the source point
float CZone::FindClosestPoint(const vector3& LocalSpacePos, float AdditionalRadius, vector3& OutClosestPoint) const
{
	const auto VertexCount = Vertices.size();
	float SqDistance;
	if (VertexCount == 0)
	{
		OutClosestPoint = vector3::Zero;
		SqDistance = std::numeric_limits<float>().max();
	}
	else if (VertexCount == 1)
	{
		OutClosestPoint = Vertices[0];
		SqDistance = vector3::SqDistance2D(LocalSpacePos, Vertices[0]);
	}
	else if (VertexCount == 2)
	{
		float t;
		SqDistance = dtDistancePtSegSqr2D(LocalSpacePos.v, Vertices[0].v, Vertices[1].v, t);
		OutClosestPoint = vector3::lerp(Vertices[0], Vertices[1], t);
	}
	else
	{
		UPTR s;
		float t;
		SqDistance = SqDistanceToPolyChain(LocalSpacePos, Vertices.data()->v, VertexCount, ClosedPolygon, s, t);
		if (s == VertexCount)
		{
			// Point is inside a closed poly
			OutClosestPoint = LocalSpacePos;
			return 0.f;
		}
		OutClosestPoint = vector3::lerp(Vertices[s], Vertices[(s + 1) % VertexCount], t);
	}

	const float CurrRadius = Radius + AdditionalRadius;
	if (SqDistance <= CurrRadius * CurrRadius)
	{
		// Point is inside a border radius
		OutClosestPoint = LocalSpacePos;
		return 0.f;
	}

	// Project outside point to the border radius
	const float Distance = n_sqrt(SqDistance);
	OutClosestPoint = vector3::lerp(OutClosestPoint, LocalSpacePos, CurrRadius / Distance);
	return Distance - CurrRadius;
}
//---------------------------------------------------------------------

// NB: OutPos is not changed if function returns false
bool CZone::IntersectsNavPoly(const matrix44& WorldTfm, float* pPolyVerts, int PolyVertCount, vector3& OutPos) const
{
	UPTR SegmentIdx = 0;
	float t = 0.f;
	float SqDistance = std::numeric_limits<float>().max();

	const auto SqRadius = Radius * Radius;
	const auto VertexCount = Vertices.size();
	if (VertexCount < 2)
	{
		const vector3 Pos = VertexCount ? WorldTfm.transform_coord(Vertices[0]) : WorldTfm.Translation();
		SqDistance = SqDistanceToPolyChain(Pos, pPolyVerts, PolyVertCount, true, SegmentIdx, t);
	}
	else if (VertexCount == 2)
	{
		// https://www.geometrictools.com/GTE/Mathematics/DistSegmentSegment.h

		//dtIntersectSegSeg2D
		//dtIntersectSegmentPoly2D(
		//	WorldTfm.transform_coord(Zone.Vertices[0]).v,
		//	WorldTfm.transform_coord(Zone.Vertices[1]).v,

		// FIXME: IMPLEMENT!
		return false;
	}
	else
	{
		// Can use separating axes to get distances?

		// FIXME: IMPLEMENT!
		return false;
	}

	if (SqDistance > SqRadius) return false;
	dtVlerp(OutPos.v, &pPolyVerts[3 * SegmentIdx], &pPolyVerts[(3 * (SegmentIdx + 1)) % VertexCount], t);
	return true;
}
//---------------------------------------------------------------------

}
