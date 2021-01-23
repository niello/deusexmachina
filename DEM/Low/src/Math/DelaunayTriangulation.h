#pragma once
#include <Math/Vector2.h>
#include <vector>
#include <tuple>
#include <numeric>
#include <algorithm>

// Delaunay triangulation algorithm

namespace DEM::Math
{

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

// [Begin, End) should not contain duplicates
template<typename TFwdIt>
void Delaunay2D(TFwdIt Begin, TFwdIt End, std::vector<std::tuple<uint32_t, uint32_t, uint32_t>>& OutTriangles)
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
	while (OutTriangles.empty() && i < Size)
	{
		const vector2 c = Tr::GetPoint(*std::next(Begin, SortedIndices[i]));
		for (uint32_t j = 0; j < i - 1; ++j)
		{
			const vector2 a = Tr::GetPoint(*std::next(Begin, SortedIndices[j]));
			const vector2 b = Tr::GetPoint(*std::next(Begin, SortedIndices[j + 1]));
			if (!IsTriangleDegenerate2D(a, b, c))
				OutTriangles.push_back({ SortedIndices[j], SortedIndices[j + 1], SortedIndices[i] });
		}
		++i;
	}
}
//---------------------------------------------------------------------

}
