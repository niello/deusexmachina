#pragma once
#include <Math/Vector2.h>
#include <vector>
#include <tuple>
#include <numeric>
#include <algorithm>

// Delaunay triangulation algorithm
// https://en.wikipedia.org/wiki/Delaunay_triangulation

namespace DEM::Math
{
constexpr uint32_t DELAUNAY_INVALID_INDEX = std::numeric_limits<uint32_t>().max();

using CDelaunayTriangle = std::tuple<uint32_t, uint32_t, uint32_t>;
//using CDelaunayTriangle = uint32_t[3];

template<typename T>
struct CDelaunayInputTraits
{
	static vector2 GetPoint(const T& Value) { return { std::get<0>(Value), std::get<1>(Value) }; }
	//???less comparison for sorting here? std::less<> by default?
};

// Triangle is degenerate if all its vertices are collinear
inline bool IsTriangleDegenerate2D(vector2 a, vector2 b, vector2 c)
{
	return (b.x - a.x) * (c.y - a.y) == (b.y - a.y) * (c.x - a.x);
}
//---------------------------------------------------------------------

// Checks if t1 and t2 share an edge with opposite winding
// OutNonSharedPoint receives a vertex of t2 not shared with t1
inline bool HasSharedEdge(CDelaunayTriangle t1, CDelaunayTriangle t2, uint32_t& OutNonSharedPoint)
{
	const auto [a1, b1, c1] = t1;
	const auto [a2, b2, c2] = t2;
	if ((a2 == b1 && b2 == a1) || (a2 == c1 && b2 == b1) || (a2 == a1 && b2 == c1)) { OutNonSharedPoint = c2; return true; }
	if ((b2 == b1 && c2 == a1) || (b2 == c1 && c2 == b1) || (b2 == a1 && c2 == c1)) { OutNonSharedPoint = a2; return true; }
	if ((c2 == b1 && a2 == a1) || (c2 == c1 && a2 == b1) || (c2 == a1 && a2 == c1)) { OutNonSharedPoint = b2; return true; }
	return false;
}
//---------------------------------------------------------------------

// Checks if t1 and t2 share an edge with the same winding
inline bool HasSameEdge(CDelaunayTriangle t1, CDelaunayTriangle t2)
{
	const auto [a1, b1, c1] = t1;
	const auto [a2, b2, c2] = t2;
	return (a2 == a1 && b2 == b1) ||
		(a2 == b1 && b2 == c1) ||
		(a2 == c1 && b2 == a1) ||
		(b2 == a1 && c2 == b1) ||
		(b2 == b1 && c2 == c1) ||
		(b2 == c1 && c2 == a1) ||
		(c2 == a1 && a2 == b1) ||
		(c2 == b1 && a2 == c1) ||
		(c2 == c1 && a2 == a1);
}
//---------------------------------------------------------------------

inline bool HasSameEdge(const std::vector<CDelaunayTriangle>& Tris, CDelaunayTriangle NewTri)
{
	for (const auto& Tri : Tris)
		if (HasSameEdge(Tri, NewTri)) return true;
	return false;
}
//---------------------------------------------------------------------

// Computes a determinant of 3x3 matrix:
// a.x-d.x; a.y-d.y; (a.x2-d.x2)+(a.y2-d.y2)
// b.x-d.x; b.y-d.y; (b.x2-d.x2)+(b.y2-d.y2)
// c.x-d.x; c.y-d.y; (c.x2-d.x2)+(c.y2-d.y2)
// TODO: Wikipedia in 2020 has a strange math for the last column: (A^2 - D^2) -> (A - D)^2 etc.
// I don't risk to use it for now, although if correct it would be faster and more compact.
inline float CircumcircleDeterminant(vector2 a, vector2 b, vector2 c, vector2 d)
{
	const vector2 da = a - d;
	const vector2 db = b - d;
	const vector2 dc = c - d;
	const float dx2 = d.x * d.x;
	const float dy2 = d.y * d.y;
	const float LastA = (a.x * a.x - dx2) + (a.y * a.y - dy2);
	const float LastB = (b.x * b.x - dx2) + (b.y * b.y - dy2);
	const float LastC = (c.x * c.x - dx2) + (c.y * c.y - dy2);
	return (da.x * db.y * LastC + da.y * LastB * dc.x + LastA * db.x * dc.y) -
		   (LastA * db.y * dc.x + da.y * db.x * LastC + da.x * LastB * dc.y);
}
//---------------------------------------------------------------------

// FIXME: flip algorithm like in UE4 just to make it work for now, rewrite with more performant algorithm?
// NB: [Begin, End) should not contain duplicates
template<typename TFwdIt>
void Delaunay2D(TFwdIt ItBegin, TFwdIt ItEnd, std::vector<CDelaunayTriangle>& OutTriangles)
{
	using Tr = CDelaunayInputTraits<typename std::iterator_traits<TFwdIt>::value_type>;

	const auto SignedSize = std::distance(ItBegin, ItEnd);
	if (SignedSize < 3)
	{
		if (SignedSize == 1)
			OutTriangles.push_back({ 0, DELAUNAY_INVALID_INDEX, DELAUNAY_INVALID_INDEX });
		else if (SignedSize == 2)
			OutTriangles.push_back({ 0, 1, DELAUNAY_INVALID_INDEX });

		return;
	}

	const size_t Size = static_cast<size_t>(SignedSize);
	std::vector<uint32_t> SortedIndices(Size);
	std::iota(SortedIndices.begin(), SortedIndices.end(), 0);
	std::sort(SortedIndices.begin(), SortedIndices.end(), [ItBegin](uint32_t ia, uint32_t ib)
	{
		const vector2 a = Tr::GetPoint(*std::next(ItBegin, ia));
		const vector2 b = Tr::GetPoint(*std::next(ItBegin, ib));
		return (a.x < b.x) || (a.x == b.x && a.y < b.y);
	});

	// Find first non-collinear triangle, also add all valid triangles for its last point
	size_t i = 2;
	for (; OutTriangles.empty() && i < Size; ++i)
	{
		const vector2 c = Tr::GetPoint(*std::next(ItBegin, SortedIndices[i]));
		for (uint32_t j = 0; j < i - 1; ++j)
		{
			const vector2 a = Tr::GetPoint(*std::next(ItBegin, SortedIndices[j]));
			const vector2 b = Tr::GetPoint(*std::next(ItBegin, SortedIndices[j + 1]));
			CDelaunayTriangle NewTri = { SortedIndices[j], SortedIndices[j + 1], SortedIndices[i] };
			if (!IsTriangleDegenerate2D(a, b, c) && !HasSameEdge(OutTriangles, NewTri))
				OutTriangles.push_back(NewTri);
		}
	}

	// Attach new triangles to existing ones
	for (; i < Size; ++i)
	{
		const auto NewIndex = SortedIndices[i];
		const vector2 New = Tr::GetPoint(*std::next(ItBegin, NewIndex));

		// Create new triangles from existing ones and a new point
		for (auto [ia, ib, ic] : OutTriangles)
		{
			const vector2 a = Tr::GetPoint(*std::next(ItBegin, ia));
			const vector2 b = Tr::GetPoint(*std::next(ItBegin, ib));
			const vector2 c = Tr::GetPoint(*std::next(ItBegin, ic));
			if (!IsTriangleDegenerate2D(a, b, New) && !HasSameEdge(OutTriangles, { ia, ib, NewIndex }))
				OutTriangles.push_back({ ia, ib, NewIndex });
			if (!IsTriangleDegenerate2D(b, c, New) && !HasSameEdge(OutTriangles, { ib, ic, NewIndex }))
				OutTriangles.push_back({ ib, ic, NewIndex });
			// FIXME: is a->c CCW? Maybe should be c->a?
			if (!IsTriangleDegenerate2D(a, c, New) && !HasSameEdge(OutTriangles, { ia, ic, NewIndex }))
				OutTriangles.push_back({ ia, ic, NewIndex });
		}

		// Triangle flipping to minimize circumcircles (maximize min angle in each tri)
		for (size_t t1 = 0; t1 < OutTriangles.size(); ++t1)
		{
			for (size_t t2 = t1 + 1; t2 < OutTriangles.size(); ++t2)
			{
				uint32_t NonSharedPointIdx;
				if (!HasSharedEdge(OutTriangles[t1], OutTriangles[t2], NonSharedPointIdx)) continue;

				//???instead of tuple store triangle as uint32_t[3]?
				uint32_t T1Vertices[3];
				std::tie(T1Vertices[0], T1Vertices[1], T1Vertices[2]) = OutTriangles[t1];

				const vector2 t1a = Tr::GetPoint(*std::next(ItBegin, T1Vertices[0]));
				const vector2 t1b = Tr::GetPoint(*std::next(ItBegin, T1Vertices[1]));
				const vector2 t1c = Tr::GetPoint(*std::next(ItBegin, T1Vertices[2]));
				const vector2 NonSharedPoint = Tr::GetPoint(*std::next(ItBegin, NonSharedPointIdx));

				// Flip triagles only if NonSharedPoint is outside the circumcircle
				const float Det = CircumcircleDeterminant(t1a, t1b, t1c, NonSharedPoint);
				if (Det <= 0.f) continue;

				CDelaunayTriangle NewTris[2];
				size_t NewTriCount = 0;
				for (uint32_t v1 = 0; v1 < 2; ++v1)
				{
					const uint32_t ia = T1Vertices[v1];
					const vector2 a = Tr::GetPoint(*std::next(ItBegin, ia));
					for (uint32_t v2 = v1 + 1; v1 < 3; ++v2)
					{
						const uint32_t ib = T1Vertices[v2];
						const vector2 b = Tr::GetPoint(*std::next(ItBegin, ib));
						if (IsTriangleDegenerate2D(a, b, NonSharedPoint)) continue;

						// Remember a new triangle if it is more optimal (c lies outside the circumcircle)
						const uint32_t ic = T1Vertices[3 - v1 - v2];
						const vector2 c = Tr::GetPoint(*std::next(ItBegin, ic));
						const float NewDet = CircumcircleDeterminant(a, b, NonSharedPoint, c);
						if (std::signbit(NewDet))
						{
							n_assert_dbg(NewTriCount < 2);
							NewTris[NewTriCount++] = { ia, ib, NonSharedPointIdx };
						}
					}
				}

				// Successfull flipping must generate 2 new triangles instead of t1 and t2
				if (NewTriCount == 2)
				{
					OutTriangles.erase(std::next(OutTriangles.begin(), t2));
					OutTriangles.erase(std::next(OutTriangles.begin(), t1));
					OutTriangles.push_back(NewTris[0]);
					OutTriangles.push_back(NewTris[1]);
					--t1;
					break;
				}
			}
		}
	}

	// If all points are collinear, connect them with segments (degenerate triangles)
	if (OutTriangles.empty())
		for (size_t i = 0; i < Size - 1; ++i)
			OutTriangles.push_back({ i, i + 1, DELAUNAY_INVALID_INDEX });
}
//---------------------------------------------------------------------

}
