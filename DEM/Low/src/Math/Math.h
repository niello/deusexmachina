#pragma once
#include <StdDEM.h>
#include <math.h>
#include <type_traits>

#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_BitScanReverse)
#if DEM_X64
#pragma intrinsic(_BitScanReverse64)
#endif
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

void InitRandomNumberGenerator();
U32 RandomU32();

// Quake inverse sqrt //???use rcpss?
//https://en.wikipedia.org/wiki/Fast_inverse_square_root
DEM_FORCE_INLINE float RSqrt(const float Value)
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

template <typename T, typename U>
DEM_FORCE_INLINE constexpr T CeilToMultiple(T x, U y) noexcept
{
	return ((x + y - 1) / y) * y;
}
//---------------------------------------------------------------------

// This version requires y to be power-of-2 but performs faster than the general one
template <typename T, typename U>
DEM_FORCE_INLINE constexpr T CeilToMultipleOfPow2(T x, U y) noexcept
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
#if DEM_X64
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
	x = (x ^ (x << 8)) & 0x00ff00ff;
	x = (x ^ (x << 4)) & 0x0f0f0f0f;
	x = (x ^ (x << 2)) & 0x33333333;
	x = (x ^ (x << 1)) & 0x55555555;
	return x;
}
//---------------------------------------------------------------------

DEM_FORCE_INLINE U64 PartBits1By1(U32 Value) noexcept
{
	U64 x = static_cast<U64>(Value);
	x = (x ^ (x << 16)) & 0x0000ffff0000ffff;
	x = (x ^ (x << 8)) & 0x00ff00ff00ff00ff;
	x = (x ^ (x << 4)) & 0x0f0f0f0f0f0f0f0f;
	x = (x ^ (x << 2)) & 0x3333333333333333;
	x = (x ^ (x << 1)) & 0x5555555555555555;
	return x;
}
//---------------------------------------------------------------------

}
