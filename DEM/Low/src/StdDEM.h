#pragma once
#include "StdCfg.h"
#include <System/Memory.h>
#include <stdint.h>
#include <vector>

#define OK				return true
#define FAIL			return false

#ifndef NULL
#define NULL			(0L)
#endif

#define INVALID_INDEX	(-1)

// http://cnicholson.net/2011/01/stupid-c-tricks-a-better-sizeof_array
template<typename T, size_t N> char (&SIZEOF_ARRAY_REQUIRES_ARRAY_ARGUMENT(T (&)[N]))[N];  
#define sizeof_array(x) sizeof(SIZEOF_ARRAY_REQUIRES_ARRAY_ARGUMENT(x))

#if defined(_MSC_VER) // __FUNCTION__ ## "()"
#   define DEM_FUNCTION_NAME __FUNCSIG__
#elif defined(__GNUC__)
#   define DEM_FUNCTION_NAME __PRETTY_FUNCTION__
#elif __STDC_VERSION__ >= 199901L
#   define DEM_FUNCTION_NAME __func__
#else
#   define DEM_FUNCTION_NAME "<DEM_FUNCTION_NAME>"
#endif

//---------------------------------------------------------------------
//  Shortcut typedefs
//---------------------------------------------------------------------

// New types

typedef intptr_t			IPTR;	// Signed integer of a pointer size
typedef uintptr_t			UPTR;	// Unsigned integer of a pointer size
typedef int8_t				I8;		// Signed 8-bit integer
typedef uint8_t				U8;		// Unsigned 8-bit integer
typedef int16_t				I16;	// Signed 16-bit integer
typedef uint16_t			U16;	// Unsigned 16-bit integer
typedef int32_t				I32;	// Signed 32-bit integer
typedef uint32_t			U32;	// Unsigned 32-bit integer
typedef int64_t				I64;	// Signed 64-bit integer
typedef uint64_t			U64;	// Unsigned 64-bit integer

typedef void*				PVOID;
typedef double				CTime;

const UPTR HalfRegisterBits	= sizeof(UPTR) << 2; // Bytes * 8 / 2
const UPTR INVALID_HALF_INDEX = (1 << HalfRegisterBits) - 1;

#define UPTR_LOW_HALF_MASK	INVALID_HALF_INDEX
#define UPTR_LOW_HALF(x)	((x) & UPTR_LOW_HALF_MASK)
#define UPTR_HIGH_HALF(x)	((x) >> HalfRegisterBits)

typedef UPTR				HHandle;
#define INVALID_HANDLE		((HHandle)(0))

//???use template C++ std facility?
#ifndef I32_MAX
#define I32_MAX				(0x7fffffff)
#endif

//---------------------------------------------------------------------
//  Compiler-dependent aliases
//---------------------------------------------------------------------
#if defined(__LINUX__) || defined(__MACOSX__)
#define n_stricmp strcasecmp
#else
#define n_stricmp _stricmp
#endif

#if defined(_MSC_VER) && _MSC_VER < 1800 // 1800 is VS2013. Not tested vith VS2010&VS2012.
#define va_copy(d, s) d = s
#endif

#ifdef __GNUC__
// Hey! Look! A cute GNU C++ extension!
#define min(a, b)   a <? b
#define max(a, b)   a >? b
#endif

//---------------------------------------------------------------------
//  Kernel and aux functions and enums
//---------------------------------------------------------------------
template<class T> inline T n_min(T a, T b) { return a < b ? a : b; }
template<class T> inline T n_max(T a, T b) { return a > b ? a : b; }
template<class T, class T2> inline T n_min(T a, T2 b) { return a < (T)b ? a : (T)b; }
template<class T, class T2> inline T n_max(T a, T2 b) { return a > (T)b ? a : (T)b; }

template <class T> inline T Clamp(T Value, T Min, T Max) { return (Value < Min) ? Min : ((Value > Max) ? Max : Value); }
inline float Saturate(float Value) { return Clamp(Value, 0.f, 1.f); }
inline bool IsPow2(unsigned int Value) { return Value > 0 && (Value & (Value - 1)) == 0; }
template <class T> inline T NextPow2(T x)
{
	// For unsigned only, else uncomment the next line
	//if (x < 0) return 0;
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

// Only for power-of-2 alignment //!!!C++11 static_assert may help!
template <unsigned int Alignment> inline bool IsAligned(const void* Pointer) { return !((unsigned int)Pointer) & (Alignment - 1); }
inline bool IsAligned16(const void* Pointer) { return !(((unsigned int)Pointer) & 0x0000000f); }

// Execution results

const UPTR Failure = 0;
const UPTR Success = 1;
const UPTR Running = 2;
const UPTR Error = 3;
// Use values bigger than Error to specify errors

inline bool ExecResultIsError(UPTR Result) { return Result >= Error; }

//

enum EClipStatus
{
	Outside,
	Inside,
	Clipped,
	//InvalidClipStatus - Clipped is used instead now
};

struct CRational
{
	I32	Numerator;
	U32	Denominator;

	I32		GetIntRounded() const { return Denominator ? Numerator / Denominator : 0; }
	float	GetFloat() const { return Denominator ? (float)Numerator / (float)Denominator : 0.f; }
	double	GetDouble() const { return Denominator ? (double)Numerator / (double)Denominator : 0.0; }

	bool operator ==(const CRational& Other) const { return Numerator == Other.Numerator && Denominator == Other.Denominator; }
	bool operator !=(const CRational& Other) const { return Numerator != Other.Numerator || Denominator != Other.Denominator; }
};

struct CRange
{
	UPTR Start;
	UPTR Count;
};
