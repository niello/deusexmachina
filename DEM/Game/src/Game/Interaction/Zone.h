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

	CFixedArray<vector3> Vertices;
	float                Radius = 0.f;
	bool                 ClosedPolygon = false; //???add also PointSet mode?

	// TODO: typical zone constructors (point, circle, line etc)

	float   FindClosestPoint(const vector3& LocalSpacePos, float AdditionalRadius, vector3& OutClosestPoint) const;
	bool    IntersectsNavPoly(const matrix44& WorldTfm, float* pPolyVerts, int PolyVertCount, vector3& OutPos) const;
};

}
