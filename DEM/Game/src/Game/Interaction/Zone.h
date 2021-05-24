#pragma once
#include <Data/FixedArray.h>
#include <Math/Matrix44.h>

// Flat zone - a set of points forming a line chain or a closed polygon, extended by radius.
// Point is inside a zone if it is inside a poly or not farther than Radius from edges.

namespace DEM::Game
{

class CZone
{
public:

	CFixedArray<vector3> Vertices; //!!!TODO: ensure convex CCW if ClosedPolygon!
	float                Radius = 0.f;
	bool                 ClosedPolygon = false; //???add also PointSet mode?

	CZone() = default;
	CZone(const vector3& Center, float Radius_ = 0.f) : Vertices(1), Radius(Radius_) { Vertices[0] = Center; }
	CZone(const vector3& a, const vector3& b, float Radius_ = 0.f) : Vertices(2), Radius(Radius_) { Vertices[0] = a; Vertices[1] = b; }

	float FindClosestPoint(const vector3& LocalSpacePos, float AdditionalRadius, vector3& OutClosestPoint) const;
	bool  IntersectsPoly(const matrix44& WorldTfm, float* pPolyVerts, int PolyVertCount) const;
};

}
