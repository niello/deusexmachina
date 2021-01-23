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
using CDelaunayTriangle = std::tuple<uint32_t, uint32_t, uint32_t>;

template<typename T>
struct CDelaunayInputTraits
{
	static vector2 GetPoint(const T& Value) { return { std::get<0>(Value), std::get<1>(Value) }; }
	//???less comparison for sorting here? std::less<> by default?
};

constexpr uint32_t DELAUNAY_INVALID_INDEX = std::numeric_limits<uint32_t>().max();

// Triangle is degenerate if all its vertices are collinear
inline bool IsTriangleDegenerate2D(vector2 a, vector2 b, vector2 c)
{
	return (b.x - a.x) * (c.y - a.y) == (b.y - a.y) * (c.x - a.x);
}
//---------------------------------------------------------------------

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

// Computes a determinant of 3x3 matrix:
// a.x-d.x; a.y-d.y; (a.x2-d.x2)+(a.y2-d.y2)
// b.x-d.x; b.y-d.y; (b.x2-d.x2)+(b.y2-d.y2)
// c.x-d.x; c.y-d.y; (c.x2-d.x2)+(c.y2-d.y2)
// TODO: Wikipedia in 2020 has a strange math for the last column: (A^2 - D^2) -> (A - D)^2 etc.
// I don't risk to use it for now, although if correct it would be faster and more compact.
template<typename TFwdIt>
inline float CircumcircleDeterminant(TFwdIt Begin, CDelaunayTriangle t, uint32_t id)
{
	using Tr = CDelaunayInputTraits<typename std::iterator_traits<TFwdIt>::value_type>;

	const auto [ia, ib, ic] = t;
	const vector2 d = Tr::GetPoint(*std::next(Begin, id));
	const vector2 a = Tr::GetPoint(*std::next(Begin, ia));
	const vector2 b = Tr::GetPoint(*std::next(Begin, ib));
	const vector2 c = Tr::GetPoint(*std::next(Begin, ic));
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

// FIXME: flip algorithm like in UE4 just to make it work for now, rewrite with more performant and well known algorithm?
// [Begin, End) should not contain duplicates
template<typename TFwdIt>
void Delaunay2D(TFwdIt Begin, TFwdIt End, std::vector<CDelaunayTriangle>& OutTriangles)
{
	using Tr = CDelaunayInputTraits<typename std::iterator_traits<TFwdIt>::value_type>;

	const auto SignedSize = std::distance(Begin, End);
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
	std::sort(SortedIndices.begin(), SortedIndices.end(), [Begin](uint32_t ia, uint32_t ib)
	{
		const vector2 a = Tr::GetPoint(*std::next(Begin, ia));
		const vector2 b = Tr::GetPoint(*std::next(Begin, ib));
		return (a.x < b.x) || (a.x == b.x && a.y < b.y);
	});

	// Find first non-collinear triangle
	size_t i = 2;
	for (; OutTriangles.empty() && i < Size; ++i)
	{
		const vector2 c = Tr::GetPoint(*std::next(Begin, SortedIndices[i]));
		for (uint32_t j = 0; j < i - 1; ++j)
		{
			const vector2 a = Tr::GetPoint(*std::next(Begin, SortedIndices[j]));
			const vector2 b = Tr::GetPoint(*std::next(Begin, SortedIndices[j + 1]));
			if (!IsTriangleDegenerate2D(a, b, c))
				OutTriangles.push_back({ SortedIndices[j], SortedIndices[j + 1], SortedIndices[i] });
		}
	}

	// Attach new triangles to existing ones
	for (; i < Size; ++i)
	{
		const auto NewIndex = SortedIndices[i];
		const vector2 New = Tr::GetPoint(*std::next(Begin, NewIndex));

		// Add a new triangle from each non-collinear edge of each existing triangle
		// FIXME: WTF? Is it Delaunay?
		for (auto [ia, ib, ic] : OutTriangles)
		{
			const vector2 a = Tr::GetPoint(*std::next(Begin, ia));
			const vector2 b = Tr::GetPoint(*std::next(Begin, ib));
			const vector2 c = Tr::GetPoint(*std::next(Begin, ic));
			if (!IsTriangleDegenerate2D(a, b, New))
				OutTriangles.push_back({ ia, ib, NewIndex });
			if (!IsTriangleDegenerate2D(b, c, New))
				OutTriangles.push_back({ ib, ic, NewIndex });
			// FIXME: is a->c CCW? Maybe should be c->a?
			if (!IsTriangleDegenerate2D(a, c, New))
				OutTriangles.push_back({ ia, ic, NewIndex });
		}

		// Local optimization (flipping)
		for (size_t t1 = 0; t1 < OutTriangles.size(); ++t1)
		{
			for (size_t t2 = t1 + 1; t2 < OutTriangles.size(); ++t2)
			{
				uint32_t NonSharedPoint;
				if (!HasSharedEdge(OutTriangles[t1], OutTriangles[t2], NonSharedPoint)) continue;

				const float Det = CircumcircleDeterminant(Begin, OutTriangles[t1], NonSharedPoint);

				// if triangles share the same edge, flip them if necessary
			}
		}
	}
}
//---------------------------------------------------------------------

}
