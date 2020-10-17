#pragma once
#include <initializer_list>
#include <limits>
#include <cmath>

// Variant data type with compile-time extendable type list

inline bool CompareFloat(float a, float b, float e = std::numeric_limits<float>().epsilon())
{
	return std::fabsf(a - b) <= e;
}
//---------------------------------------------------------------------

//???use 32-bit ACL/RTM vectors?
struct float2
{
	union
	{
		float v[2];
		struct
		{
			float x, y;
		};
	};

	constexpr float2() : x(0.f), y(0.f) {}
	constexpr float2(float x_, float y_) : x(x_), y(y_) {}

	bool operator ==(const float2& Other) const { return CompareFloat(x, Other.x) && CompareFloat(y, Other.y); }
	bool operator !=(const float2& Other) const { return !(*this == Other); }
};

//???use 32-bit ACL/RTM vectors?
struct float3
{
	union
	{
		float v[3];
		struct
		{
			float x, y, z;
		};
	};

	constexpr float3() : x(0.f), y(0.f), z(0.f) {}
	constexpr float3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

	constexpr float3(const float* pFloats, size_t Count)
		: x(Count > 0 ? pFloats[0] : 0.f)
		, y(Count > 1 ? pFloats[1] : 0.f)
		, z(Count > 2 ? pFloats[2] : 0.f)
	{}

	bool operator ==(const float3& Other) const { return CompareFloat(x, Other.x) && CompareFloat(y, Other.y) && CompareFloat(z, Other.z); }
	bool operator !=(const float3& Other) const { return !(*this == Other); }
};

//???use 32-bit ACL/RTM vectors?
struct float4
{
	union
	{
		float v[4];
		struct
		{
			float x, y, z, w;
		};
	};

	constexpr float4() : x(0.f), y(0.f), z(0.f), w(0.f) {}
	constexpr float4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
	constexpr float4(std::initializer_list<float> List) : float4(List.begin(), List.size()) {}

	float4(const float3& Other) : x(Other.x), y(Other.y), z(Other.z), w(0.f) {}
	float4(const float4& Other) = default;
	float4(float4&& Other) = default;

	constexpr float4(const float* pFloats, size_t Count)
		: x(Count > 0 ? pFloats[0] : 0.f)
		, y(Count > 1 ? pFloats[1] : 0.f)
		, z(Count > 2 ? pFloats[2] : 0.f)
		, w(Count > 3 ? pFloats[3] : 0.f)
	{}

	float4& operator =(const float4& Other) = default;
	float4& operator =(float4&& Other) = default;

	bool operator ==(const float4& Other) const { return CompareFloat(x, Other.x) && CompareFloat(y, Other.y) && CompareFloat(z, Other.z) && CompareFloat(w, Other.w); }
	bool operator !=(const float4& Other) const { return !(*this == Other); }
};
