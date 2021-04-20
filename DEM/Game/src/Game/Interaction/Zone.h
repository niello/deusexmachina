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
	bool                 ClosedPolygon = false;

	// TODO: typical zone constructors (point, circle, line etc)

	// TODO: better method names!
	float   CalcSqDistance(const vector3& Pos, UPTR& OutSegment, float& OutT) const;
	vector3 GetPoint(const vector3& Pos, UPTR Segment, float t) const;
	bool    IntersectsNavPoly(const matrix44& WorldTfm, float* pPolyVerts, int PolyVertCount, vector3& OutPos) const;
};

}
