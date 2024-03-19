#pragma once
#include <StdDEM.h>
#include <math.h>
#include <type_traits>

#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_BitScanReverse)
#if DEM_CPU_64
#pragma intrinsic(_BitScanReverse64)
#endif
#endif

#if DEM_BMI2 || DEM_F16C
#include <immintrin.h>
#endif

// Declarations and utility functions

#if defined(_MSC_VER) && (_MSC_VER < 1800) // Not tested with other versions
#define isnan _isnan
#define isinf _isinf
#endif

#ifndef PI
#define PI (3.1415926535897932384626433832795028841971693993751f)
#endif

#ifndef TWO_PI
#define TWO_PI (6.2831853071795864769252867665590f)
#endif

#ifndef HALF_PI
#define HALF_PI (1.5707963267948966192313216916398f)
#endif

#ifndef INV_PI
#define INV_PI (0.31830988618379067153776752677335f)
#endif

constexpr float COS_PI_DIV_4 = 0.70710678118654752440084436210485f;

#ifndef TINY
#define TINY (0.0000001f)
#endif

#define M_RAN_INVM32 2.32830643653869628906e-010f // For random number conversion from int to float

const float LN_2 = 0.693147180559945f;
const float INV_LN_2 = 1.442695040888964f;

#define n_abs(a)        (((a)<0.0f) ? (-(a)) : (a))
#define n_sgn(a)        (((a)<0.0f) ? (-1) : (1))
#define n_deg2rad(d)    (((d)*PI)/180.0f)
#define n_rad2deg(r)    ((r)*180.0f*INV_PI)
#define n_sin(x)        (float(sin(x)))
#define n_cos(x)        (float(cos(x)))
#define n_tan(x)        (float(tan(x)))
#define n_atan(x)       (float(atan(x)))
#define n_atan2(x,y)    (float(atan2(x,y)))
#define n_exp(x)        (float(exp(x)))
#define n_floor(x)      (float(floor(x)))
#define n_ceil(x)       (float(ceil(x)))
#define n_pow(x,y)      (float(pow(x,y)))

inline float n_log2(float f) { return logf(f) * INV_LN_2; }
inline bool n_fless(float f0, float f1, float tol) { return f0 - f1 < tol; }
inline bool n_fgreater(float f0, float f1, float tol) { return f0 - f1 > tol; }
inline float n_fmod(float x, float y) { return fmodf(x, y); }

// acos with value clamping.
inline float n_acos(float x)
{
	if (x > 1.f) x = 1.f;
	else if (x < -1.f) x = -1.f;
	return acosf(x);
}
//---------------------------------------------------------------------

// asin with value clamping.
inline float n_asin(float x)
{
	if (x > 1.f) x = 1.f;
	else if (x < -1.f) x = -1.f;
	return asinf(x);
}
//---------------------------------------------------------------------

inline float n_sqrt(float x)
{
	if (x < 0.f) x = 0.f;
	return sqrtf(x);
}
//---------------------------------------------------------------------

// fast float to int conversion (always truncates)
// see http://www.stereopsis.com/FPU.html for a discussion.
// NB: this works only on x86 endian machines.
inline long n_ftol(float val)
{
	double v = double(val) + (68719476736.0 * 1.5);
	return ((long*)&v)[0] >> 16;
}
//---------------------------------------------------------------------

inline int n_frnd(float f)
{
	return n_ftol(f + 0.5f);
}
//---------------------------------------------------------------------

#define n_fabs fabsf
/*
// fast float absolute value
// NB: this relies on IEEE floating point standard.
//???mb there is a FPU instruction and this method isn't preferable?
inline float n_fabs(float val)
{
	int tmp = (int&)val & 0x7FFFFFFF;
	return (float&)tmp;
}
//---------------------------------------------------------------------
*/

inline bool n_fequal(float f0, float f1, float tol = 0.00001f)
{
	return n_fabs(f0 - f1) < tol;
}
//---------------------------------------------------------------------

// Smooth a new value towards an old value using a change value.
inline float n_smooth(float newVal, float curVal, float maxChange)
{
    float diff = newVal - curVal;
    if (n_fabs(diff) > maxChange)
    {
        if (diff > 0.0f)
        {
            curVal += maxChange;
            if (curVal > newVal) curVal = newVal;
        }
        else if (diff < 0.0f)
        {
            curVal -= maxChange;
            if (curVal < newVal) curVal = newVal;
        }
    }
    else curVal = newVal;
    return curVal;
}
//---------------------------------------------------------------------

inline float n_lerp(float x, float y, float l)
{
	return x + l * (y - x);
}
//---------------------------------------------------------------------

// Template-based linear interpolation function that can be specialized for any type
template<class T>
inline void lerp(T & result, const T& val0, const T& val1, float lerpVal)
{
	Sys::Error("Unimplemented lerp function!");
}
//---------------------------------------------------------------------

template<> inline void lerp<int>(int& result, const int& val0, const int& val1, float lerpVal)
{
	result = n_frnd((float)val0 + (((float)val1 - (float)val0) * lerpVal));
}
//---------------------------------------------------------------------

template<> inline void lerp<float>(float& result, const float& val0, const float& val1, float lerpVal)
{
	result = n_lerp(val0, val1, lerpVal);
}
//---------------------------------------------------------------------

inline void n_sincos(float Angle, float& Sin, float& Cos)
{
	Sin = n_sin(Angle);
	Cos = n_sqrt(1.f - Sin * Sin);
	float Quarter = n_fmod(n_fabs(Angle), TWO_PI);
	if (Quarter > 0.5f * PI && Quarter < 1.5f * PI) Cos = -Cos;
}
//---------------------------------------------------------------------

inline void n_sincos_square(float Angle, float& SqSin, float& SqCos)
{
	SqSin = n_sin(Angle);
	SqSin *= SqSin;
	SqCos = 1.f - SqSin;
}
//---------------------------------------------------------------------

// Normalize an angular value into the range 0 to 2 PI.
inline float n_normangle(float a)
{
	while (a < 0.f) a += TWO_PI;
	if (a >= TWO_PI) a = n_fmod(a, TWO_PI);
	return a;
}
//---------------------------------------------------------------------

// Normalize an angular value into the range -PI to PI.
inline float n_normangle_signed_pi(float a)
{
	while (a < -PI) a += TWO_PI;
	while (a > PI) a -= TWO_PI;
	return a;
}
//---------------------------------------------------------------------

// Compute the shortest angular distance between 2 angles. The angular distance
// will be between -PI and PI. Positive distance are in counter-clockwise
// order, negative distances in clockwise order.
inline float n_angulardistance(float from, float to)
{
	float normFrom = n_normangle(from);
	float normTo   = n_normangle(to);
	float dist = normTo - normFrom;
	if (dist < -PI) dist += TWO_PI;
	else if (dist > PI) dist -= TWO_PI;
	return dist;
}
//---------------------------------------------------------------------

namespace Math
{
U32 RandomU32();

// Quake inverse sqrt //???use rcpss?
//https://en.wikipedia.org/wiki/Fast_inverse_square_root
DEM_FORCE_INLINE float RSqrt(float Value)
{
	const float Half = Value * 0.5f;
	float y = Value;
	U32 i = *(U32*)&y;
	i = 0x5f3759df - (i >> 1);
	y = *(float*)&i;
	return y * (1.5f - (Half * y * y));
}

// Solves ax^2 + bx + c = 0 equation. Returns a number of real roots and optionally root values.
inline UPTR SolveQuadraticEquation(float a, float b, float c, float* pOutX1 = nullptr, float* pOutX2 = nullptr)
{
	const float D = b * b - 4.f * a * c;

	if (D < 0.f) return 0;

	if (n_fequal(D, 0.f))
	{
		if (pOutX1)
		{
			*pOutX1 = -b / (2.f * a);
			if (pOutX2) *pOutX2 = *pOutX1;
		}
		return 1;
	}

	const float SqrtD = n_sqrt(D);
	const float InvDenom = 0.5f / a;
	if (pOutX1) *pOutX1 = (-b - SqrtD) * InvDenom;
	if (pOutX2) *pOutX2 = (SqrtD - b) * InvDenom;
	return 2;
}
//---------------------------------------------------------------------

// Returns random float in [0; 1) range
DEM_FORCE_INLINE float RandomFloat()
{
	return static_cast<I32>(RandomU32()) * M_RAN_INVM32 + 0.5f;
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE float RandomFloat(float Min, float Max)
{
	return Min + RandomFloat() * (Max - Min);
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE U32 RandomU32(U32 Min, U32 Max)
{
	return Min + RandomU32() % (Max - Min + 1);
}
//---------------------------------------------------------------------

// Integer pow. See https://stackoverflow.com/questions/101439/the-most-efficient-way-to-implement-an-integer-based-power-function-powint-int.
template <typename T, typename U, typename = std::enable_if_t<std::is_integral_v<T>&& std::is_integral_v<U>>>
DEM_FORCE_INLINE T Pow(T Base, U Exp)
{
	T Result = 1;
	for (;;)
	{
		if (Exp & 1) Result *= Base;
		Exp >>= 1;
		if (!Exp) break;
		Base *= Base;
	}

	return Result;
}
//---------------------------------------------------------------------

// Divide and round up
template <typename T, typename U, typename = std::enable_if_t<std::is_integral_v<T>&& std::is_integral_v<U>>>
DEM_FORCE_INLINE decltype(auto) DivCeil(T Numerator, U Denominator)
{
	return (Numerator + Denominator - 1) / Denominator;
}
//---------------------------------------------------------------------

template <typename T, typename U>
DEM_FORCE_INLINE constexpr decltype(auto) CeilToMultiple(T x, U y) noexcept
{
	return DivCeil(x, y) * y;
}
//---------------------------------------------------------------------

// This version requires y to be power-of-2 but performs faster than the general one
template <typename T, typename U>
DEM_FORCE_INLINE constexpr decltype(auto) CeilToMultipleOfPow2(T x, U y) noexcept
{
	return (x + (y - 1)) & ~(y - 1);
}
//---------------------------------------------------------------------

template <typename T>
DEM_FORCE_INLINE constexpr T CeilToEven(T x) noexcept
{
	return x + (x & 1);
}
//---------------------------------------------------------------------

template <typename T>
DEM_FORCE_INLINE constexpr bool IsPow2(T x) noexcept
{
	return x > 0 && (x & (x - 1)) == 0;
}
//---------------------------------------------------------------------

template <typename T>
DEM_FORCE_INLINE constexpr T NextPow2(T x) noexcept
{
#if __cplusplus >= 202002L
	return std::bit_ceil(x);
#else
	if constexpr (std::is_signed_v<T>)
	{
		if (x < 0) return 0;
	}

	--x;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);

	if constexpr (sizeof(T) > 4)
		x |= (x >> 32);

	return x + 1;
#endif
}
//---------------------------------------------------------------------

template <typename T>
DEM_FORCE_INLINE constexpr T PrevPow2(T x) noexcept
{
#if __cplusplus >= 202002L
	return std::bit_floor(x);
#else
	if constexpr (std::is_signed_v<T>)
	{
		if (x <= 0) return 0;
	}
	else
	{
		if (!x) return 0;
	}

	//--x; // Uncomment this to have a strictly less result
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);

	if constexpr (sizeof(T) > 4)
		x |= (x >> 32);

	return x - (x >> 1);
#endif
}
//---------------------------------------------------------------------

template <typename T>
DEM_FORCE_INLINE constexpr int CountLeadingZeros(T x) noexcept
{
	static_assert(sizeof(T) <= 8 && std::is_unsigned_v<T> && std::is_integral_v<T> && !std::is_same_v<T, bool>);

#if __cplusplus >= 202002L
	return std::countl_zero(x);
#elif defined(__GNUC__)
	if constexpr (sizeof(T) == sizeof(unsigned long long))
		return __builtin_clzll(x);
	else if constexpr (sizeof(T) == sizeof(unsigned long))
		return __builtin_clzl(x);
	else
		return __builtin_clz(x);
#elif defined(_MSC_VER)
	unsigned long Bit;
#if DEM_CPU_64
	if constexpr (sizeof(T) == 8)
	{
		if (!_BitScanReverse64(&Bit, x)) Bit = -1;
		return 63 - Bit;
	}
	if constexpr (sizeof(T) == 4)
	{
		_BitScanReverse64(&Bit, static_cast<uint64_t>(x) * 2 + 1);
		return 32 - Bit;
	}
#else
	if constexpr (sizeof(T) == 8)
	{
		int Bits = 64;
		while (x)
		{
			--Bits;
			x >>= 1;
		}
		return Bits;
	}
	if constexpr (sizeof(T) == 4)
	{
		if (!_BitScanReverse(&Bit, x)) Bit = -1;
		return 31 - Bit;
	}
#endif
	if constexpr (sizeof(T) < 4)
	{
		_BitScanReverse(&Bit, static_cast<uint32_t>(x) * 2 + 1);
		return static_cast<T>(sizeof(T) * 8 - Bit);
	}
#else
	int Bits = sizeof(T) * 8;
	while (x)
	{
		--Bits;
		x >>= 1;
	}
	return Bits;
#endif
}
//---------------------------------------------------------------------

template <typename T>
DEM_FORCE_INLINE constexpr int BitWidth(T x) noexcept
{
#if __cplusplus >= 202002L
	return std::bit_width(x);
#else
	return std::numeric_limits<T>::digits - CountLeadingZeros(x);
#endif
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE U32 PartBits1By1(U16 Value) noexcept
{
	U32 x = static_cast<U32>(Value);
#if DEM_BMI2
	return _pdep_u32(x, 0x55555555);
#else
	x = (x ^ (x << 8)) & 0x00ff00ff;
	x = (x ^ (x << 4)) & 0x0f0f0f0f;
	x = (x ^ (x << 2)) & 0x33333333;
	x = (x ^ (x << 1)) & 0x55555555;
	return x;
#endif
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE U16 CompactBits1By1(U32 x) noexcept
{
#if DEM_BMI2
	return _pext_u32(x, 0x55555555);
#else
	x &= 0x55555555;
	x = (x ^ (x >> 1)) & 0x33333333;
	x = (x ^ (x >> 2)) & 0x0f0f0f0f;
	x = (x ^ (x >> 4)) & 0x00ff00ff;
	x = (x ^ (x >> 8)) & 0x0000ffff;
	return static_cast<U16>(x);
#endif
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE U64 PartBits1By1(U32 Value) noexcept
{
	U64 x = static_cast<U64>(Value);
#if DEM_BMI2 && DEM_CPU_64
	return _pdep_u64(x, 0x5555555555555555);
#else
	x = (x ^ (x << 16)) & 0x0000ffff0000ffff;
	x = (x ^ (x << 8)) & 0x00ff00ff00ff00ff;
	x = (x ^ (x << 4)) & 0x0f0f0f0f0f0f0f0f;
	x = (x ^ (x << 2)) & 0x3333333333333333;
	x = (x ^ (x << 1)) & 0x5555555555555555;
	return x;
#endif
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE U32 CompactBits1By1(U64 x) noexcept
{
#if DEM_BMI2 && DEM_CPU_64
	return _pext_u64(x, 0x5555555555555555);
#else
	x &= 0x5555555555555555;
	x = (x ^ (x >> 1)) & 0x3333333333333333;
	x = (x ^ (x >> 2)) & 0x0f0f0f0f0f0f0f0f;
	x = (x ^ (x >> 4)) & 0x00ff00ff00ff00ff;
	x = (x ^ (x >> 8)) & 0x0000ffff0000ffff;
	x = (x ^ (x >> 16)) & 0x00000000ffffffff;
	return static_cast<U32>(x);
#endif
}
//---------------------------------------------------------------------

// NB: max 10 bits allowed for a Value
DEM_FORCE_INLINE U32 PartBits1By2(U16 Value) noexcept
{
	U32 x = static_cast<U32>(Value);
	//x &= 0x000003ff; - explicitly clamp to 10 bits
#if DEM_BMI2
	return _pdep_u32(x, 0x09249249);
#else
	x = (x ^ (x << 16)) & 0x30000ff;
	x = (x ^ (x << 8)) & 0x0300f00f;
	x = (x ^ (x << 4)) & 0x030c30c3;
	x = (x ^ (x << 2)) & 0x09249249;
	return x;
#endif
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE U16 CompactBits1By2(U32 x) noexcept
{
#if DEM_BMI2
	return _pext_u32(x, 0x09249249);
#else
	x &= 0x09249249;
	x = (x ^ (x >> 2)) & 0x030c30c3;
	x = (x ^ (x >> 4)) & 0x0300f00f;
	x = (x ^ (x >> 8)) & 0x30000ff;
	x = (x ^ (x >> 16)) & 0x000003ff;
	return static_cast<U16>(x);
#endif
}
//---------------------------------------------------------------------

// NB: max 21 bit allowed for a Value
DEM_FORCE_INLINE U64 PartBits1By2(U32 Value) noexcept
{
	U64 x = static_cast<U64>(Value);
	//x &= 0x1fffff; - explicitly clamp to 21 bits
#if DEM_BMI2 && DEM_CPU_64
	return _pdep_u64(x, 0x9249249249249249);
#else
	x = (x ^ (x << 32)) & 0x1f00000000ffff;
	x = (x ^ (x << 16)) & 0x1f0000ff0000ff;
	x = (x ^ (x << 8)) & 0x100f00f00f00f00f;
	x = (x ^ (x << 4)) & 0x10c30c30c30c30c3;
	x = (x ^ (x << 2)) & 0x1249249249249249;
	return x;
#endif
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE U32 CompactBits1By2(U64 x) noexcept
{
#if DEM_BMI2 && DEM_CPU_64
	return _pext_u64(x, 0x9249249249249249);
#else
	x &= 0x1249249249249249;
	x = (x ^ (x >> 2)) & 0x10c30c30c30c30c3;
	x = (x ^ (x >> 4)) & 0x100f00f00f00f00f;
	x = (x ^ (x >> 8)) & 0x1f0000ff0000ff;
	x = (x ^ (x >> 16)) & 0x1f00000000ffff;
	x = (x ^ (x >> 32)) & 0x1fffff;
	return static_cast<U32>(x);
#endif
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE U32 MortonCode2(U16 x, U16 y) noexcept
{
#if DEM_BMI2
	return _pdep_u32(x, 0x55555555) | _pdep_u32(y, 0xaaaaaaaa);
#elif DEM_CPU_64
	const U32 Mixed = (static_cast<U32>(y) << 16) | x;
	const U64 MixedParted = Math::PartBits1By1(Mixed);
	return static_cast<U32>((MixedParted >> 31) | (MixedParted & 0x0ffffffff));
#else
	return Math::PartBits1By1(x) | (Math::PartBits1By1(y) << 1);
#endif
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE void MortonDecode2(U32 Code, U16& x, U16& y) noexcept
{
#if DEM_BMI2
	x = _pext_u32(x, 0x55555555);
	y = _pext_u32(y, 0xaaaaaaaa);
//#elif DEM_CPU_64 //!!!TODO!
//	const U32 Mixed = (static_cast<U32>(y) << 16) | x;
//	const U64 MixedParted = Math::PartBits1By1(Mixed);
//	return static_cast<U32>((MixedParted >> 31) | (MixedParted & 0x0ffffffff));
#else
	x = Math::CompactBits1By1(Code);
	y = Math::CompactBits1By1(Code >> 1);
#endif
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE U64 MortonCode2(U32 x, U32 y) noexcept
{
#if DEM_BMI2 && DEM_CPU_64
	return _pdep_u64(x, 0x5555555555555555) | _pdep_u64(y, 0xaaaaaaaaaaaaaaaa);
#else
	return Math::PartBits1By1(x) | (Math::PartBits1By1(y) << 1);
#endif
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE void MortonDecode2(U64 Code, U32& x, U32& y) noexcept
{
#if DEM_BMI2
	x = _pext_u64(x, 0x5555555555555555);
	y = _pext_u64(y, 0xaaaaaaaaaaaaaaaa);
#else
	x = Math::CompactBits1By1(Code);
	y = Math::CompactBits1By1(Code >> 1);
#endif
}
//---------------------------------------------------------------------

// NB: max 10 bits allowed per component
DEM_FORCE_INLINE U32 MortonCode3_10bit(U16 x, U16 y, U16 z) noexcept
{
#if DEM_BMI2
	return _pdep_u32(x, 0x09249249) | _pdep_u32(y, 0x12492492) | _pdep_u32(z, 0x24924924);
#else
	return Math::PartBits1By2(x) | (Math::PartBits1By2(y) << 1) | (Math::PartBits1By2(z) << 2);
#endif
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE void MortonDecode3(U32 Code, U16& x, U16& y, U16& z) noexcept
{
#if DEM_BMI2
	x = _pext_u32(x, 0x09249249);
	y = _pext_u32(y, 0x12492492);
	z = _pext_u32(y, 0x24924924);
#else
	x = Math::CompactBits1By2(Code);
	y = Math::CompactBits1By2(Code >> 1);
	z = Math::CompactBits1By2(Code >> 2);
#endif
}
//---------------------------------------------------------------------

// NB: max 21 bits allowed per component
DEM_FORCE_INLINE U64 MortonCode3_21bit(U32 x, U32 y, U32 z) noexcept
{
#if DEM_BMI2 && DEM_CPU_64
	return _pdep_u64(x, 0x9249249249249249) | _pdep_u64(y, 0x2492492492492492) | _pdep_u64(z, 0x4924924924924924);
#else
	return Math::PartBits1By2(x) | (Math::PartBits1By2(y) << 1) | (Math::PartBits1By2(z) << 2);
#endif
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE void MortonDecode3(U64 Code, U32& x, U32& y, U32& z) noexcept
{
#if DEM_BMI2
	x = _pext_u64(x, 0x9249249249249249);
	y = _pext_u64(y, 0x2492492492492492);
	z = _pext_u64(y, 0x4924924924924924);
#else
	x = Math::CompactBits1By2(Code);
	y = Math::CompactBits1By2(Code >> 1);
	z = Math::CompactBits1By2(Code >> 2);
#endif
}
//---------------------------------------------------------------------

// Finds the Least Common Ancestor of two nodes represented by Morton codes
template<size_t DIMENSIONS, typename T>
DEM_FORCE_INLINE T MortonLCA(T MortonCodeA, T MortonCodeB) noexcept
{
	// Shrink longer code so that both codes represent nodes on the same level
	const auto Bits1 = BitWidth(MortonCodeA);
	const auto Bits2 = BitWidth(MortonCodeB);
	if (Bits1 < Bits2)
		MortonCodeB >>= (Bits2 - Bits1);
	else
		MortonCodeA >>= (Bits1 - Bits2);

	// LCA is the equal prefix of both nodes. Find the place where equality breaks.
	auto HighestUnequalBit = BitWidth(MortonCodeA ^ MortonCodeB);

	// Each level uses DIMENSIONS bits and we must shift by whole levels
	if constexpr (DIMENSIONS == 2)
		HighestUnequalBit = CeilToEven(HighestUnequalBit);
	else if constexpr (IsPow2(DIMENSIONS))
		HighestUnequalBit = CeilToMultipleOfPow2(HighestUnequalBit, DIMENSIONS);
	else
		HighestUnequalBit = CeilToMultiple(HighestUnequalBit, DIMENSIONS);

	// Shift any of codes to obtain the common prefix which is the LCA of two nodes
	return MortonCodeA >> HighestUnequalBit;
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE UPTR GetQuadtreeNodeCount(UPTR Depth)
{
	return (Pow(4, Depth) - 1) / 3;
}
//---------------------------------------------------------------------

// See https://en.wikipedia.org/wiki/Half-precision_floating-point_format
DEM_FORCE_INLINE uint16_t FloatToHalf(float Value)
{
#if DEM_F16C
	return static_cast<uint16_t>(_mm_extract_epi16(_mm_cvtps_ph(_mm_set_ss(Value), _MM_FROUND_TO_NEAREST_INT), 0));
#else
	uint32_t SourceBits = reinterpret_cast<uint32_t&>(Value);

	// Extract the sign bit from the source
	const uint32_t Sign = (SourceBits & 0x80000000);
	SourceBits &= 0x7fffffff;

	uint32_t Result;
	if (SourceBits >= 0x47800000) // e+16, too large for half, make inf, -inf or NaN
	{
		Result = 0x7c00;
		if (SourceBits > 0x7f800000) Result |= (0x200 | ((SourceBits >> 13) & 0x3ff));
	}
	else if (SourceBits <= 0x33000000) // e-25, too small for half, make 0
	{
		Result = 0;
	}
	else if (SourceBits < 0x38800000) // e-14, too small for a normalized half
	{
		// Make subnormal
		const uint32_t Shift = 125 - (SourceBits >> 23);
		SourceBits = 0x800000 | (SourceBits & 0x7fffff);
		Result = SourceBits >> (Shift + 1);
		const uint32_t HasShiftBits = (SourceBits & ((1 << Shift) - 1)) != 0;
		Result += (Result | HasShiftBits) & ((SourceBits >> Shift) & 1);
	}
	else // Can be a normalized half
	{
		// Bias the exponent
		SourceBits += 0xc8000000;
		Result = ((SourceBits + 0x0fff + ((SourceBits >> 13) & 1)) >> 13) & 0x7fff;
	}

	return static_cast<uint16_t>(Result | (Sign >> 16));
#endif
}
//---------------------------------------------------------------------

// See https://en.wikipedia.org/wiki/Half-precision_floating-point_format
DEM_FORCE_INLINE float HalfToFloat(uint16_t Value)
{
#if DEM_F16C
	return _mm_cvtss_f32(_mm_cvtph_ps(_mm_cvtsi32_si128(static_cast<int>(Value))));
#else
	auto Mantissa = static_cast<uint32_t>(Value & 0x03ff);
	auto Exponent = static_cast<uint32_t>(Value & 0x7c00);

	if (Exponent == 0x7c00)
	{
		// inf, -inf or NaN
		Exponent = 0xff;
	}
	else if (Exponent)
	{
		// Normal numbers
		Exponent = static_cast<uint32_t>((Value >> 10) & 0x1f) + 0x70;
	}
	else if (!Mantissa)
	{
		// Zero
		Exponent = 0;
	}
	else
	{
		// Subnormal number, normalize it
		Exponent = 0x71;
		do
		{
			--Exponent;
			Mantissa <<= 1;
		}
		while (!(Mantissa & 0x400));

		Mantissa &= 0x03ff;
	}

	const uint32_t ResultBits = ((Value & 0x8000) << 16) | (Exponent << 23) | (Mantissa << 13);
	return reinterpret_cast<const float&>(ResultBits);
#endif
}
//---------------------------------------------------------------------

}
