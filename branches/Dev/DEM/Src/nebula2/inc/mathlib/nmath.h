#ifndef N_MATH_H
#define N_MATH_H
//------------------------------------------------------------------------------
/**
    General math functions and macros.

    (C) 2003 RadonLabs GmbH

    - 07-Jun-05    kims    Added 'n_atan2', 'n_exp', 'n_floor', 'n_ceil' and
                           'n_pow' macros.
*/
#include <math.h>
#include <stdlib.h>	// rand

#include <kernel/ntypes.h>

#ifdef _MSC_VER
#define isnan _isnan
#define isinf _isinf
#endif

#ifndef PI
#define PI (3.1415926535897932384626433832795028841971693993751f)
#define INV_PI (0.31830988618379067153776752677335f)
#endif

#ifndef TINY
#define TINY (0.0000001f)
#endif

template<class T> inline T n_min(T a, T b) { return a < b ? a : b; }
template<class T> inline T n_max(T a, T b) { return a > b ? a : b; }
template<class T, class T2> inline T n_min(T a, T2 b) { return a < (T)b ? a : (T)b; }
template<class T, class T2> inline T n_max(T a, T2 b) { return a > (T)b ? a : (T)b; }

#define n_abs(a)        (((a)<0.0f) ? (-(a)) : (a))
#define n_sgn(a)        (((a)<0.0f) ? (-1) : (1))
#define n_deg2rad(d)    (((d)*PI)/180.0f)
#define n_rad2deg(r)    (((r)*180.0f)/PI)
#define n_sin(x)        (float(sin(x)))
#define n_cos(x)        (float(cos(x)))
#define n_tan(x)        (float(tan(x)))
#define n_atan(x)       (float(atan(x)))
#define n_atan2(x,y)    (float(atan2(x,y)))
#define n_exp(x)        (float(exp(x)))
#define n_floor(x)      (float(floor(x)))
#define n_ceil(x)       (float(ceil(x)))
#define n_pow(x,y)      (float(pow(x,y)))

/**
    log2() function.
*/
const float LN_2 = 0.693147180559945f;
inline float n_log2(float f)
{
    return logf(f) / LN_2;
}

//------------------------------------------------------------------------------
/**
    Integer clamping.
*/
inline int n_iclamp(int val, int minVal, int maxVal)
{
    if (val < minVal)      return minVal;
    else if (val > maxVal) return maxVal;
    else return val;
}

//------------------------------------------------------------------------------
/**
    acos with value clamping.
*/
inline float n_acos(float x)
{
    if (x >  1.0f) x =  1.0f;
    if (x < -1.0f) x = -1.0f;
    return (float)acos(x);
}

//------------------------------------------------------------------------------
/**
    asin with value clamping.
*/
inline float n_asin(float x)
{
    if (x >  1.0f) x =  1.0f;
    if (x < -1.0f) x = -1.0f;
    return (float)asin(x);
}

//------------------------------------------------------------------------------
/**
    Safe sqrt.
*/
inline float n_sqrt(float x)
{
    if (x < 0.0f) x = (float) 0.0f;
    return (float) sqrt(x);
}

//------------------------------------------------------------------------------
/**
    A fuzzy floating point equality check
*/
inline bool n_fequal(float f0, float f1, float tol) {
    float f = f0-f1;
    if ((f>(-tol)) && (f<tol)) return true;
    else                       return false;
}

//------------------------------------------------------------------------------
/**
    A fuzzy floating point less-then check.
*/
inline bool n_fless(float f0, float f1, float tol) {
    if ((f0-f1)<tol) return true;
    else             return false;
}

//------------------------------------------------------------------------------
/**
    A fuzzy floating point greater-then check.
*/
inline bool n_fgreater(float f0, float f1, float tol) {
    if ((f0-f1)>tol) return true;
    else             return false;
}

//------------------------------------------------------------------------------
/**
    fast float to int conversion (always truncates)
    see http://www.stereopsis.com/FPU.html for a discussion.
    NOTE: this works only on x86 endian machines.
*/
inline long n_ftol(float val)
{
    double v = double(val) + (68719476736.0*1.5);
    return ((long*)&v)[0] >> 16;
}

//------------------------------------------------------------------------------
/**
    fast float absolute value
    NOTE: this relies on IEEE floating point standard.
*/
inline float n_fabs(float val)
{
	int tmp = (int&)val & 0x7FFFFFFF;
	return (float&)tmp;
}

//------------------------------------------------------------------------------
/**
    Smooth a new value towards an old value using a change value.
*/
inline float n_smooth(float newVal, float curVal, float maxChange)
{
    float diff = newVal - curVal;
    if (n_fabs(diff) > maxChange)
    {
        if (diff > 0.0f)
        {
            curVal += maxChange;
            if (curVal > newVal)
            {
                curVal = newVal;
            }
        }
        else if (diff < 0.0f)
        {
            curVal -= maxChange;
            if (curVal < newVal)
            {
                curVal = newVal;
            }
        }
    }
    else
    {
        curVal = newVal;
    }
    return curVal;
}

//------------------------------------------------------------------------------
/**
    Clamp a value against lower und upper boundary.
*/
inline float n_clamp(float val, float lower, float upper)
{
    if (val < lower)      return lower;
    else if (val > upper) return upper;
    else                  return val;
}

//------------------------------------------------------------------------------
/**
    Saturate a value (clamps between 0.0f and 1.0f)
*/
inline float n_saturate(float val)
{
    if (val < 0.0f)      return 0.0f;
    else if (val > 1.0f) return 1.0f;
    else return val;
}

//------------------------------------------------------------------------------
/**
    Return a pseudo random number between 0 and 1.
*/
inline float n_rand()
{
    return float(rand()) / float(RAND_MAX);
}

//------------------------------------------------------------------------------
/**
    Return a pseudo random number between min and max.
*/
inline float n_rand(float min, float max)
{
    return min + (float(rand()) / RAND_MAX) * (max - min);
}

//------------------------------------------------------------------------------
/**
    Return a pseudo random number between min and max.
*/
inline int n_rand_int(int min, int max)
{
    return min + rand() % (max - min + 1);
}

//------------------------------------------------------------------------------
/**
    Chop float to int.
*/
inline int n_fchop(float f)
{
    // FIXME!
    return int(f);
}

//------------------------------------------------------------------------------
/**
    Round float to integer.
*/
inline int n_frnd(float f)
{
    return n_fchop(floorf(f + 0.5f));
}

//------------------------------------------------------------------------------
/**
    Linearly interpolate between 2 values: ret = x + l * (y - x)
*/
inline float n_lerp(float x, float y, float l)
{
    return x + l * (y - x);
}

//------------------------------------------------------------------------------
/**
*/
inline
float
n_fmod(float x, float y)
{
    return fmodf(x, y);
}

//------------------------------------------------------------------------------
/**
    Template-based linear interpolation function used by nIpolKeyArray that
    can be specialized for any type.
*/
template<class TYPE>
inline
void
lerp(TYPE & result, const TYPE & val0, const TYPE & val1, float lerpVal)
{
    n_error("Unimplemented lerp function!");
}

//------------------------------------------------------------------------------
/**
*/
template<>
inline
void
lerp<int>(int & result, const int & val0, const int & val1, float lerpVal)
{
    result = n_frnd((float)val0 + (((float)val1 - (float)val0) * lerpVal));
}

//------------------------------------------------------------------------------
/**
*/
template<>
inline
void
lerp<float>(float & result, const float & val0, const float & val1, float lerpVal)
{
    result = val0 + ((val1 - val0) * lerpVal);
}

inline void n_sincos(float Angle, float& Sin, float& Cos)
{
	Sin = n_sin(Angle);
	Cos = n_sqrt(1.f - Sin * Sin);
	float Quarter = n_fmod(n_fabs(Angle), 2.f * PI);
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

//------------------------------------------------------------------------------
/**
    Normalize an angular value into the range rad(0) to rad(360).
*/
inline float n_normangle(float a)
{
    while (a < 0.0f)
    {
        a += n_deg2rad(360.0f);
    }
    if (a >= n_deg2rad(360.0f))
    {
        a = n_fmod(a, n_deg2rad(360.0f));
    }
    return a;
}
//---------------------------------------------------------------------

// Compute the shortest angular distance between 2 angles. The angular distance
// will be between rad(-180) and rad(180). Positive distance are in counter-clockwise
// order, negative distances in clockwise order.
inline float n_angulardistance(float from, float to)
{
	float normFrom = n_normangle(from);
	float normTo   = n_normangle(to);
	float dist = normTo - normFrom;
	if (dist < n_deg2rad(-180.0f))
		dist += n_deg2rad(360.0f);
	else if (dist > n_deg2rad(180.0f))
		dist -= n_deg2rad(360.0f);
	return dist;
}
//---------------------------------------------------------------------

#endif

