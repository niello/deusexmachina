#include <rtm/vector4f.h>
#include <vector>

// At least part of the code is from recastnavigation\RecastDemo\Source\ConvexVolumeTool.cpp

// Returns true if 'c' is left of line 'a'-'b'.
static inline bool left(const float* a, const float* b, const float* c)
{ 
	const float u1 = b[0] - a[0];
	const float v1 = b[2] - a[2];
	const float u2 = c[0] - a[0];
	const float v2 = c[2] - a[2];
	return u1 * v2 - v1 * u2 < 0;
}
//---------------------------------------------------------------------

// Returns true if 'a' is more lower-left than 'b'.
static inline bool cmppt(const float* a, const float* b)
{
	if (a[0] < b[0]) return true;
	if (a[0] > b[0]) return false;
	if (a[2] < b[2]) return true;
	if (a[2] > b[2]) return false;
	return false;
}
//---------------------------------------------------------------------

// Calculates convex hull on xz-plane of points on 'pts',
// stores the indices of the resulting hull in 'out' and
// returns number of points on hull.
static std::vector<size_t> ConvexHull(const std::vector<rtm::vector4f>& In)
{
	// Find lower-leftmost point.
	size_t hull = 0;
	for (size_t i = 1; i < In.size(); ++i)
		if (cmppt(rtm::vector_to_pointer(In[i]), rtm::vector_to_pointer(In[hull])))
			hull = i;

	std::vector<size_t> Result;

	// Gift wrap hull.
	size_t endpt = 0;
	do
	{
		Result.push_back(hull);
		endpt = 0;
		for (size_t i = 1; i < In.size(); ++i)
		{
			if (hull == endpt ||
				left(rtm::vector_to_pointer(In[hull]),
					rtm::vector_to_pointer(In[endpt]),
					rtm::vector_to_pointer(In[i])))
			{
				endpt = i;
			}
		}
		hull = endpt;
	}
	while (endpt != Result[0]);

	return Result;
}
//---------------------------------------------------------------------

std::vector<float> OffsetPoly(const float* pSrcVerts, int SrcCount, float Offset)
{
	std::vector<float> Result;

	if (!pSrcVerts || !SrcCount) return Result;

	std::vector<rtm::vector4f> SrcVerts(SrcCount);
	for (int i = 0; i < SrcCount; ++i)
		SrcVerts[i] = { pSrcVerts[i * 3], pSrcVerts[i * 3 + 1], pSrcVerts[i * 3 + 2], 0.f };

	rtm::vector4f Center = rtm::vector_zero();
	for (auto SrcVertex : SrcVerts)
		Center = rtm::vector_add(Center, SrcVertex);
	Center = rtm::vector_div(Center, { static_cast<float>(SrcCount), static_cast<float>(SrcCount), static_cast<float>(SrcCount) });

	std::vector<rtm::vector4f> TmpVerts(SrcCount);
	for (int i = 0; i < SrcCount; ++i)
	{
		TmpVerts[i] = rtm::vector_sub(SrcVerts[i], Center);
		const float Len = rtm::vector_length3(TmpVerts[i]);
		if (Len > 0.f)
		{
			const float Coeff = std::max(0.f, Len + Offset) / Len;
			TmpVerts[i] = rtm::vector_mul_add(TmpVerts[i], Coeff, Center);
		}
	}

	const auto Hull = ConvexHull(TmpVerts);

	// Degenerate region
	if (Hull.size() < 3) return Result;

	Result.reserve(Hull.size() * 3);
	for (auto i : Hull)
	{
		const auto& Vertex = TmpVerts[Hull[i]];
		Result.push_back(rtm::vector_get_x(Vertex));
		Result.push_back(rtm::vector_get_y(Vertex));
		Result.push_back(rtm::vector_get_z(Vertex));
	}

	return Result;
}
//---------------------------------------------------------------------

bool GetPointInPoly(const std::vector<float>& In, float Out[3])
{
	// Need at least 3 vertices, 3 floats each
	if (In.size() < 9) return false;

	rtm::vector4f a = { In[0], In[1], In[2], 0.f };
	a = rtm::vector_add(a, { In[3], In[4], In[5], 0.f });
	a = rtm::vector_add(a, { In[6], In[7], In[8], 0.f });
	a = rtm::vector_div(a, rtm::vector_set(3.f));
	Out[0] = rtm::vector_get_x(a);
	Out[1] = rtm::vector_get_y(a);
	Out[2] = rtm::vector_get_z(a);
	return true;
}
//---------------------------------------------------------------------
