#pragma once
#ifndef __DEM_L1_MATH_H__
#define __DEM_L1_MATH_H__

#include <StdDEM.h>
#include <math.h>
#include <stdlib.h>	// rand //???get some cool random number generator?

// Declarations and utility functions

//???different min, max and clamp functions here?

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

#define n_fabs fabs
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

inline bool n_fequal(float f0, float f1, float tol)
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

// Return a pseudo random number between 0 and 1.
inline float n_rand()
{
	return float(rand()) / float(RAND_MAX);
}
//---------------------------------------------------------------------

inline float n_rand(float min, float max)
{
	return min + (float(rand()) / RAND_MAX) * (max - min);
}
//---------------------------------------------------------------------

inline int n_rand_int(int min, int max)
{
	return min + rand() % (max - min + 1);
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

// Solves ax^2 + bx + c = 0 equation. Returns a number of real roots and optionally root values.
inline DWORD SolveQuadraticEquation(float a, float b, float c, float* pOutX1 = NULL, float* pOutX2 = NULL)
{
	float D = b * b - 4.f * a * c;

	if (D < 0.f) return 0;

	if (D == 0.f)
	{
		if (pOutX1) *pOutX1 = -b / (2.f * a);
		if (pOutX2) *pOutX2 = *pOutX1;
		return 1;
	}

	float SqrtD = n_sqrt(D);
	float InvDenom = 0.5f / a;
	if (pOutX1) *pOutX1 = (-b - SqrtD) * InvDenom;
	if (pOutX2) *pOutX2 = (SqrtD - b) * InvDenom;
	return 2;
}
//---------------------------------------------------------------------

}

#endif
