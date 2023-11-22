#include "Zone.h"
#include <Math/SIMDMath.h>

namespace DEM::Game
{

static bool RTM_SIMD_CALL PointInPolygon(rtm::vector4f_arg0 pt, const rtm::vector4f* verts, const int nverts)
{
	const float ptx = rtm::vector_get_x(pt);
	const float ptz = rtm::vector_get_z(pt);

	bool c = false;
	for (int i = 0, j = nverts - 1; i < nverts; j = i++)
	{
		const float vix = rtm::vector_get_x(verts[i * 3]);
		const float viz = rtm::vector_get_z(verts[i * 3]);
		const float vjx = rtm::vector_get_x(verts[j * 3]);
		const float vjz = rtm::vector_get_z(verts[j * 3]);
		if (((viz > ptz) != (vjz > ptz)) && (ptx < (vjx - vix) * (ptz - viz) / (vjz - viz) + vix))
			c = !c;
	}
	return c;
}
//---------------------------------------------------------------------

// NB: if closed, must be convex
static float SqDistanceToPolyChain(const rtm::vector4f& Pos, const rtm::vector4f* pVertices, UPTR VertexCount, bool Closed, UPTR& OutSegment, float& OutT)
{
	float MinSqDistance = std::numeric_limits<float>().max();
	if (Closed)
	{
		// NB: only convex polys are supported for now!
		if (PointInPolygon(Pos, pVertices, VertexCount))
		{
			OutSegment = VertexCount;
			return 0.f;
		}

		// Process implicit closing edge
		MinSqDistance = Math::DistancePtSegSqr2D(Pos, pVertices[VertexCount - 1], pVertices[0], OutT);
		OutSegment = VertexCount - 1;
	}

	for (UPTR i = 0; i < VertexCount - 1; ++i)
	{
		float t;
		const float SqDistance = Math::DistancePtSegSqr2D(Pos, pVertices[i], pVertices[i + 1], t);
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
float CZone::FindClosestPoint(const rtm::vector4f& LocalSpacePos, float AdditionalRadius, rtm::vector4f& OutClosestPoint) const
{
	const auto VertexCount = Vertices.size();
	float SqDistance;
	if (VertexCount == 0)
	{
		OutClosestPoint = rtm::vector_zero();
		SqDistance = Math::vector_length_squared_xz(LocalSpacePos); // Distance from zero is the length
	}
	else if (VertexCount == 1)
	{
		OutClosestPoint = Vertices[0];
		SqDistance = Math::vector_distance_squared_xz(LocalSpacePos, Vertices[0]);
	}
	else if (VertexCount == 2)
	{
		float t;
		SqDistance = Math::DistancePtSegSqr2D(LocalSpacePos, Vertices[0], Vertices[1], t);
		OutClosestPoint = rtm::vector_lerp(Vertices[0], Vertices[1], t);
	}
	else
	{
		UPTR s;
		float t;
		SqDistance = SqDistanceToPolyChain(LocalSpacePos, Vertices.data(), VertexCount, ClosedPolygon, s, t);
		if (s == VertexCount)
		{
			// Point is inside a closed poly
			OutClosestPoint = LocalSpacePos;
			return 0.f;
		}
		OutClosestPoint = rtm::vector_lerp(Vertices[s], Vertices[(s + 1) % VertexCount], t);
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
	OutClosestPoint = rtm::vector_lerp(OutClosestPoint, LocalSpacePos, CurrRadius / Distance);
	return Distance - CurrRadius;
}
//---------------------------------------------------------------------

// NB: poly vertices must be CCW
bool CZone::IntersectsPoly(const rtm::matrix3x4f& WorldTfm, rtm::vector4f* pPolyVerts, int PolyVertCount) const
{
	UPTR SegmentIdx = 0;
	float t = 0.f;
	float SqDistance = std::numeric_limits<float>().max();

	const auto VertexCount = Vertices.size();
	if (VertexCount < 2)
	{
		// Test point in poly
		const rtm::vector4f Pos = VertexCount ? rtm::matrix_mul_point3(Vertices[0], WorldTfm) : WorldTfm.w_axis;
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
		//NOT_IMPLEMENTED;
		return false;
	}
	else
	{
		// Can use separating axes to get distances?

		// FIXME: IMPLEMENT!
		//NOT_IMPLEMENTED;
		return false;
	}

	return SqDistance <= Radius * Radius;
}
//---------------------------------------------------------------------

}
