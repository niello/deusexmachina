#pragma once
#ifndef __DEM_STDDEM_H__
#define __DEM_STDDEM_H__

#include <stdlib.h>		// malloc, free
#include <new>			// operator new
#include "StdCfg.h"
#include <Core/Core.h>	// Platform-independent core functions, like Core::ReportAssertionFailure

#define OK				return true
#define FAIL			return false

#ifndef NULL
#define NULL			(0L)
#endif

#define INVALID_INDEX	(-1)

#define N_MAXPATH		(512)		// maximum length for complete path
#define N_WHITESPACE	" \r\n\t"

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

#define APP_STATE_EXIT		CStrID("Exit")

#ifndef MAX_DWORD
#define MAX_DWORD			(0xffffffff)
#endif

#ifndef MAX_SDWORD
#define MAX_SDWORD			(0x7fffffff)
#endif

//---------------------------------------------------------------------
//  Debug macros
//
// This code uses:
// http://cnicholson.net/2009/02/stupid-c-tricks-adventures-in-assert/
//---------------------------------------------------------------------

#define DBG_BREAK() __debugbreak()

#ifdef DEM_NO_ASSERT
	#define n_verify(exp)			do { (exp); } while(0)
	#define n_assert(exp)			do { (void)sizeof(exp); } while(0)
	#define n_assert2(exp, msg)		do { (void)sizeof(exp); } while(0)
#else
#define n_verify(exp)				do { if (!(exp)) if (Core::ReportAssertionFailure(#exp, NULL, __FILE__, __LINE__)) DBG_BREAK(); } while(0)
	#define n_assert(exp)			do { if (!(exp)) if (Core::ReportAssertionFailure(#exp, NULL, __FILE__, __LINE__)) DBG_BREAK(); } while(0)
	#define n_assert2(exp, msg)		do { if (!(exp)) if (Core::ReportAssertionFailure(#exp, msg, __FILE__, __LINE__)) DBG_BREAK(); } while(0)
#endif

#ifdef _DEBUG
	#define n_verify_dbg(exp)		n_verify(exp)
	#define n_assert_dbg(exp)		n_assert(exp)
	#define n_assert2_dbg(exp, msg)	n_assert2(exp, msg)
	#define n_printf_dbg(msg, ...)	n_printf(msg, __VA_ARGS__)
#else
	#define n_verify_dbg(exp)		do { (exp); } while(0)
	#define n_assert_dbg(exp)		do { (void)sizeof(exp); } while(0)
	#define n_assert2_dbg(exp, msg)	do { (void)sizeof(exp); } while(0)
	#define n_printf_dbg(fmt, ...)
#endif

//------------------------------------------------------------------------------
//  Nebula memory management and debugging stuff.
//------------------------------------------------------------------------------
extern bool DEM_LogMemory;
struct nMemoryStats
{
	int HighWaterSize;      // max allocated size so far
	int TotalCount;         // total number of allocations
	int TotalSize;          // current allocated size
};

int n_dbgmemdumpleaks();
void n_dbgmeminit();                // initialize memory debugging system
nMemoryStats n_dbgmemgetstats();    // defined in ndbgalloc.cc

#ifdef new
#undef new
#endif

#ifdef delete
#undef delete
#endif

// implemented in DbgAlloc.cpp
void* operator new(size_t size);
void* operator new(size_t size, const char* file, int line);
void* operator new(size_t size, void* place, const char* file, int line);
void* operator new[](size_t size);
void* operator new[](size_t size, const char* file, int line);
void operator delete(void* p);
void operator delete(void* p, const char* file, int line);
void operator delete(void*, void*, const char* file, int line);
void operator delete[](void* p);
void operator delete[](void* p, const char* file, int line);
void* n_malloc_dbg(size_t size, const char* file, int line);
void* n_calloc_dbg(size_t num, size_t size, const char* file, int line);
void* n_realloc_dbg(void* memblock, size_t size, const char* file, int line);
void n_free_dbg(void* memblock, const char* file, int line);

#if defined(_DEBUG) && defined(__WIN32__)
#define n_new(type) new(__FILE__,__LINE__) type
#define n_placement_new(place, type) new(place, __FILE__,__LINE__) type
#define n_new_array(type,size) new(__FILE__,__LINE__) type[size]
#define n_delete(ptr) delete ptr
#define n_delete_array(ptr) delete[] ptr
#define n_malloc(size) n_malloc_dbg(size, __FILE__, __LINE__)
#define n_calloc(num, size) n_calloc_dbg(num, size, __FILE__, __LINE__)
#define n_realloc(memblock, size) n_realloc_dbg(memblock, size, __FILE__, __LINE__)
#define n_free(memblock) n_free_dbg(memblock, __FILE__, __LINE__)
#else
#define n_new(type) new type
#define n_placement_new(place, type) new(place) type
#define n_new_array(type,size) new type[size]
#define n_delete(ptr) delete ptr
#define n_delete_array(ptr) delete[] ptr
#define n_malloc(size) malloc(size)
#define n_calloc(num, size) calloc(num, size)
#define n_realloc(memblock, size) realloc(memblock, size)
#define n_free(memblock) free(memblock)
#endif

#define SAFE_RELEASE(n)			if (n) {(n)->Release(); (n)=NULL;}
#define SAFE_DELETE(n)			if (n) {n_delete (n); (n) = NULL;}
#define SAFE_DELETE_ARRAY(n)	if (n) {n_delete_array(n); (n) = NULL;}
#define SAFE_FREE(n)			if (n) {n_free(n); (n) = NULL;}

#define CAST(Pointer, Type)((Type*)((PVOID)(Pointer)))

//---------------------------------------------------------------------
//  Compiler-dependent aliases
//---------------------------------------------------------------------
#if defined(__LINUX__) || defined(__MACOSX__)
#define n_stricmp strcasecmp
#else
#define n_stricmp stricmp
#endif

#ifdef _MSC_VER
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
void __cdecl n_printf(const char*, ...)
    __attribute__((format(printf, 1, 2)));
void __cdecl n_message(const char*, ...)
    __attribute__((format(printf, 1, 2)));
void __cdecl n_dbgout(const char*, ...)
    __attribute__((format(printf, 1, 2)));
void n_sleep(double);
char* n_strdup(const char*);
char* n_strncpy2(char*, const char*, size_t);
bool n_strmatch(const char*, const char*);
void n_strcat(char*, const char*, size_t);

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

#endif
