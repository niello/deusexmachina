#pragma once
#ifndef __DEM_STDDEM_H__
#define __DEM_STDDEM_H__

#include "StdCfg.h"
#include <System/Memory.h>

#define OK				return true
#define FAIL			return false

#ifndef NULL
#define NULL			(0L)
#endif

#define INVALID_INDEX	(-1)

// http://cnicholson.net/2011/01/stupid-c-tricks-a-better-sizeof_array
template<typename T, size_t N> char (&SIZEOF_ARRAY_REQUIRES_ARRAY_ARGUMENT(T (&)[N]))[N];  
#define sizeof_array(x) sizeof(SIZEOF_ARRAY_REQUIRES_ARRAY_ARGUMENT(x))

//---------------------------------------------------------------------
//  Shortcut typedefs
//---------------------------------------------------------------------
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef float               FLOAT;
typedef int                 INT;
typedef unsigned int        UINT;
typedef unsigned __int64	QWORD;	//!!!???Large_integer?!
//typedef LARGE_INTEGER		LINT;
typedef DWORD				FOURCC;
typedef long				HRESULT;
typedef char*				LPSTR;
typedef const char*			LPCSTR;
typedef void*				PVOID;
typedef unsigned long		ulong;
typedef unsigned int		uint;
typedef unsigned short		ushort;
typedef unsigned char		uchar;
typedef double				CTime;

#ifndef MAX_DWORD
#define MAX_DWORD			(0xffffffff)
#endif

#ifndef MAX_SDWORD
#define MAX_SDWORD			(0x7fffffff)
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
char* n_strdup(const char*);
bool n_strmatch(const char*, const char*);

template<class T> inline T n_min(T a, T b) { return a < b ? a : b; }
template<class T> inline T n_max(T a, T b) { return a > b ? a : b; }
template<class T, class T2> inline T n_min(T a, T2 b) { return a < (T)b ? a : (T)b; }
template<class T, class T2> inline T n_max(T a, T2 b) { return a > (T)b ? a : (T)b; }

template <class T> inline T Clamp(T Value, T Min, T Max) { return (Value < Min) ? Min : ((Value > Max) ? Max : Value); }
inline float Saturate(float Value) { return Clamp(Value, 0.f, 1.f); }

inline bool IsPow2(int Value) { return Value >= 1 && (Value & (Value - 1)) == 0; }

// Execution results

const DWORD Failure = 0;
const DWORD Success = 1;
const DWORD Running = 2;
const DWORD Error = 3;
// Use values bigger than Error to specify errors

inline bool ExecResultIsError(DWORD Result) { return Result >= Error; }

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
	short	Numerator;
	ushort	Denominator;

	float	GetFloat() const { return Denominator ? (float)Numerator / (float)Denominator : 0.f; }
	double	GetDouble() const { return Denominator ? (double)Numerator / (double)Denominator : 0.0; }

	bool operator ==(const CRational& Other) const { return Numerator == Other.Numerator && Denominator == Other.Denominator; }
	bool operator !=(const CRational& Other) const { return Numerator != Other.Numerator || Denominator != Other.Denominator; }
};

#endif
