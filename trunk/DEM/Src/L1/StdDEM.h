#pragma once
#ifndef __DEM_STDDEM_H__ //!!!to L0 later!
#define __DEM_STDDEM_H__

#define OK		return true
#define FAIL	return false

//
#define NULL    0

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

inline FOURCC MakeFOURCC(LPCSTR String)
{
	return	((DWORD)(BYTE)(String[0]))|
			((DWORD)(BYTE)(String[1])<<8)|
			((DWORD)(BYTE)(String[2])<<16)|
			((DWORD)(BYTE)(String[3])<<24);
}

inline DWORD MB(DWORD MBytes){return MBytes*1048576;}
//

const int DEM_THREAD_COUNT = 1;

#define INVALID_INDEX	(-1)

#define APP_STATE_EXIT	CStrID("Exit")

#ifndef MAX_DWORD
#define MAX_DWORD		(0xffffffff)
#endif

#ifndef MAX_SDWORD
#define MAX_SDWORD		(0x7fffffff)
#endif

#define SAFE_RELEASE(n)			if (n) {(n)->Release(); (n)=NULL;}
#define SAFE_DELETE(n)			if (n) {delete (n); (n) = NULL;}
#define SAFE_DELETE_ARRAY(n)	if (n) {delete[] (n); (n) = NULL;}
#define SAFE_FREE(n)			if (n) {n_free(n); (n) = NULL;}

#define CAST(Pointer, Type)((Type*)((PVOID)(Pointer)))

template <class T> inline T Clamp(T Value, T Min, T Max){return (Value<Min)?Min:((Value>Max)?Max:Value);}

enum EExecStatus
{
	Failure = 0,
	Success = 1,
	Running = 2,
	Error = 3		// Keep Error the last in the list, so you can use return value like (Error + ERRCODE)
};

enum EClipStatus
{
	InvalidClipStatus,
	Inside,
	Outside,
	Clipped
};

#include "StdCfg.h"

#endif
