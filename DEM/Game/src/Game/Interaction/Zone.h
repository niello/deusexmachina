#pragma once
#include <Data/FixedArray.h>
#include <Data/Metadata.h>
#include <rtm/matrix3x4f.h>

// Flat zone - a set of points forming a line chain or a closed polygon, extended by radius.
// Point is inside a zone if it is inside a poly or not farther than Radius from edges.

namespace DEM::Game
{

class CZone
{
public:

	CFixedArray<rtm::vector4f> Vertices; //!!!TODO: ensure convex CCW if ClosedPolygon!
	float                      Radius = 0.f;
	bool                       ClosedPolygon = false; //???add also PointSet mode?

	CZone() = default;
	CZone(const rtm::vector4f& Center, float Radius_ = 0.f) : Vertices(1), Radius(Radius_) { Vertices[0] = Center; }
	CZone(const rtm::vector4f& a, const rtm::vector4f& b, float Radius_ = 0.f) : Vertices(2), Radius(Radius_) { Vertices[0] = a; Vertices[1] = b; }

	float FindClosestPoint(const rtm::vector4f& LocalSpacePos, float AdditionalRadius, rtm::vector4f& OutClosestPoint) const;
	bool  IntersectsPoly(const rtm::matrix3x4f& WorldTfm, rtm::vector4f* pPolyVerts, int PolyVertCount) const;
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<Game::CZone>() { return "DEM::Game::CZone"; }
template<> inline constexpr auto RegisterMembers<Game::CZone>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(Game::CZone, Vertices),
		DEM_META_MEMBER_FIELD(Game::CZone, Radius),
		DEM_META_MEMBER_FIELD(Game::CZone, ClosedPolygon)
	);
}

}
