#pragma once
#ifndef __DEM_STDCFG_H__
#define __DEM_STDCFG_H__

#define DEM_STATS
//#define DEM_NO_ASSERT

const int DEM_THREAD_COUNT = 1;

#define DEM_MAX_PATH	(512)		// Maximum length for complete path
#define DEM_WHITESPACE	" \r\n\t"

#ifdef __WIN32__
#undef __WIN32__
#endif

#ifdef WIN32
#   define __WIN32__ (1)
#endif

#ifdef __WIN32__
#   ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#   endif

    // this speeds up windows.h processing dramatically...
#   define NOGDICAPMASKS
#   define OEMRESOURCE
#   define NOATOM
#   define NOCLIPBOARD
//#   define NOCTLMGR // :\ typedef LPCDLGTEMPLATE PROPSHEETPAGE_RESOURCE; fails in WinSDK 7.0
#   define NOMEMMGR
#   define NOMETAFILE
#   define NOOPENFILE
#   define NOSERVICE
#   define NOSOUND
#   define NOCOMM
#   define NOKANJI
#   define NOHELP
#   define NOPROFILER
#   define NODEFERWINDOWPOS
#   define NOMCX
#endif

// extralean flags
// NOIME, NORPC, NOPROXYSTUB, NOIMAGE, NOTAPE

//------------------------------------------------------------------------------
//  compiler identification:
//  __VC__      -> Visual C
//  __GNUC__    -> gcc
//------------------------------------------------------------------------------
#ifdef _MSC_VER
#define __VC__ (1)
#endif

//------------------------------------------------------------------------------
//  Disable some Visual C++ warnings
//  NEVER add another warning ignores if you aren't the project lead
//------------------------------------------------------------------------------
#ifdef __VC__
#pragma warning(disable : 4355)	// initialization list uses 'this'
#pragma warning(disable : 4530)	// C++ exception handler used, but unwind semantics not enabled
#endif

//------------------------------------------------------------------------------
// Only do __attributes__ for GCC.
//------------------------------------------------------------------------------
#ifndef __GNUC__
#  define  __attribute__(x)  /**/
#endif

#endif
