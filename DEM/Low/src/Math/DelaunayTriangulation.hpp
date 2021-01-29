#pragma once
#include <Math/Vector2.h>
#include <vector>
#include <array>   // std::array, for assignment and {}-init
#include <numeric> // std::iota

// Delaunay triangulation algorithm
// https://en.wikipedia.org/wiki/Delaunay_triangulation

namespace DEM::Math
{
constexpr uint32_t DelaunaySuperTriAIndex = std::numeric_limits<uint32_t>().max() - 3;
constexpr uint32_t DelaunaySuperTriBIndex = std::numeric_limits<uint32_t>().max() - 2;
constexpr uint32_t DelaunaySuperTriCIndex = std::numeric_limits<uint32_t>().max() - 1;
constexpr uint32_t DELAUNAY_INVALID_INDEX = std::numeric_limits<uint32_t>().max();

template<typename T>
struct CDelaunayInputTraits
{
	static vector2 GetPoint(const T& Value) { return { std::get<0>(Value), std::get<1>(Value) }; }
};

inline vector2 SuperTriangle(uint32_t Index)
{
	constexpr float BIG_FLT = 1.e10f;
	constexpr vector2 DelaunaySuperTriA(0.f, BIG_FLT);
	constexpr vector2 DelaunaySuperTriB(-BIG_FLT, -BIG_FLT);
	constexpr vector2 DelaunaySuperTriC(BIG_FLT, -BIG_FLT);

	switch (Index)
	{
		case DelaunaySuperTriAIndex: return DelaunaySuperTriA;
		case DelaunaySuperTriBIndex: return DelaunaySuperTriB;
		case DelaunaySuperTriCIndex: return DelaunaySuperTriC;
		default: return {};
	}
}
//---------------------------------------------------------------------

// Computes a determinant of 3x3 matrix:
// a.x-d.x; a.y-d.y; (a.x2-d.x2)+(a.y2-d.y2)
// b.x-d.x; b.y-d.y; (b.x2-d.x2)+(b.y2-d.y2)
// c.x-d.x; c.y-d.y; (c.x2-d.x2)+(c.y2-d.y2)
// Result > 0.f - d is inside a circumsircle, == 0.f - on it, < 0.f - outside. 
// TODO: Wikipedia in 2020 has strange math for the last column: (A^2 - D^2) -> (A - D)^2 etc.
// I don't risk to use it for now, although if correct it would be faster and more compact.
inline float CircumcircleDeterminant(vector2 a, vector2 b, vector2 c, vector2 d)
{
	const float dax = a.x - d.x;
	const float day = a.y - d.y;
	const float dbx = b.x - d.x;
	const float dby = b.y - d.y;
	const float dcx = c.x - d.x;
	const float dcy = c.y - d.y;
	const float dx2 = d.x * d.x;
	const float dy2 = d.y * d.y;
	const float LastA = (a.x * a.x - dx2) + (a.y * a.y - dy2);
	const float LastB = (b.x * b.x - dx2) + (b.y * b.y - dy2);
	const float LastC = (c.x * c.x - dx2) + (c.y * c.y - dy2);
	return (dax * dby - day * dbx) * LastC + (day * dcx - dax * dcy) * LastB + (dbx * dcy - dby * dcx) * LastA;
}
//---------------------------------------------------------------------

// Naive Bowyer-Watson implementation. Result is CCW. There are possible optimizations to be added later if necessary.
// NB: input vertices should not contain duplicates
template<typename TFwdIt>
bool Delaunay2D(TFwdIt ItBegin, TFwdIt ItEnd, std::vector<std::array<uint32_t, 3>>& OutTriangles)
{
	using Tr = CDelaunayInputTraits<typename std::iterator_traits<TFwdIt>::value_type>;

	const auto SignedSize = std::distance(ItBegin, ItEnd);
	if (SignedSize < 3)
	{
		if (SignedSize == 1)
			OutTriangles.push_back({ 0, DELAUNAY_INVALID_INDEX, DELAUNAY_INVALID_INDEX });
		else if (SignedSize == 2)
			OutTriangles.push_back({ 0, 1, DELAUNAY_INVALID_INDEX });

		return false;
	}
	const auto Size = static_cast<size_t>(SignedSize);

	// Max index must be less than the first special index
	if (Size > DelaunaySuperTriAIndex) return;

	OutTriangles.clear();
	OutTriangles.reserve(Size + 1);
	OutTriangles.push_back({ DelaunaySuperTriAIndex, DelaunaySuperTriBIndex, DelaunaySuperTriCIndex });

	// Used inside a loop iteration. Declared outside to avoid reallocations.
	using CEdge = std::array<uint32_t, 2>;
	std::vector<std::pair<CEdge, bool>> Edges; // Edge -> valid (non-shared)
	Edges.reserve(24);

	auto ItPoint = ItBegin;
	for (size_t i = 0; i < Size; ++i, ++ItPoint)
	{
		const vector2 pt = Tr::GetPoint(*ItPoint);

		// Partition valid and invalid triangles
		const auto ItInvalidBegin = std::partition(OutTriangles.begin(), OutTriangles.end(), [pt, Size, ItBegin](const auto& Tri)
		{
			const vector2 a = (Tri[0] < Size) ? Tr::GetPoint(*std::next(ItBegin, Tri[0])) : SuperTriangle(Tri[0]);
			const vector2 b = (Tri[1] < Size) ? Tr::GetPoint(*std::next(ItBegin, Tri[1])) : SuperTriangle(Tri[1]);
			const vector2 c = (Tri[2] < Size) ? Tr::GetPoint(*std::next(ItBegin, Tri[2])) : SuperTriangle(Tri[2]);
			return CircumcircleDeterminant(a, b, c, pt) <= 0.f;
		});

		// Find the boundary of the polygonal hole caused by removing invalid triangles
		for (auto ItInvalid = ItInvalidBegin; ItInvalid != OutTriangles.end(); ++ItInvalid)
		{
			const auto& Tri = *ItInvalid;
			bool Edge01 = true;
			bool Edge12 = true;
			bool Edge20 = true;

			// Test with inverted winding
			// NB: only index is checked, that's why it is better to filter out duplicate input points
			for (auto& [Edge, IsValid] : Edges)
			{
				if (Edge[0] == Tri[0] && Edge[1] == Tri[2]) IsValid = Edge20 = false;
				else if (Edge[0] == Tri[1] && Edge[1] == Tri[0]) IsValid = Edge01 = false;
				else if (Edge[0] == Tri[2] && Edge[1] == Tri[1]) IsValid = Edge12 = false;
				else continue;
				if (!Edge01 && !Edge12 && !Edge20) break;
			}

			if (Edge01) Edges.push_back({ { Tri[0], Tri[1] }, true });
			if (Edge12) Edges.push_back({ { Tri[1], Tri[2] }, true });
			if (Edge20) Edges.push_back({ { Tri[2], Tri[0] }, true });
		}

		size_t NewTriIdx = static_cast<size_t>(std::distance(OutTriangles.begin(), ItInvalidBegin));
		OutTriangles.resize(NewTriIdx + Edges.size());
		for (const auto [Edge, IsValid] : Edges)
			if (IsValid)
				OutTriangles[NewTriIdx++] = { Edge[0], Edge[1], i };
		OutTriangles.resize(NewTriIdx);

		Edges.clear();
	}

	// Remove triangles with super-triangle vertices
	OutTriangles.erase(std::remove_if(OutTriangles.begin(), OutTriangles.end(), [Size](const auto& Tri)
	{
		return Tri[0] >= Size || Tri[1] >= Size || Tri[2] >= Size;
	}), OutTriangles.end());

	if (!OutTriangles.empty()) return true;

	// If all points are collinear, connect them with segments (degenerate triangles)
	// Sort points in 2D to minimize segments and avoid overlapping
	std::vector<uint32_t> SortedIndices(Size);
	std::iota(SortedIndices.begin(), SortedIndices.end(), 0);
	std::sort(SortedIndices.begin(), SortedIndices.end(), [ItBegin](uint32_t ia, uint32_t ib)
	{
		const vector2 a = Tr::GetPoint(*std::next(ItBegin, ia));
		const vector2 b = Tr::GetPoint(*std::next(ItBegin, ib));
		return (a.x < b.x) || (a.x == b.x && a.y < b.y);
	});

	for (size_t i = 0; i < Size - 1; ++i)
		OutTriangles.push_back({ SortedIndices[i], SortedIndices[i + 1], DELAUNAY_INVALID_INDEX });

	return false;
}
//---------------------------------------------------------------------

}
