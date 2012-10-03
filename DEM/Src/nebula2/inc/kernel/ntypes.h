#ifndef N_TYPES_H
#define N_TYPES_H
//------------------------------------------------------------------------------
/**
    Lowlevel Nebula defines.

    (C) 2002 RadonLabs GmbH
*/
#ifndef __XBxX__
#include <errno.h>
#include <stdio.h>
#include <new>
#endif

#include "kernel/nsystem.h"
#include "kernel/ndebug.h"

// Shortcut Typedefs
typedef unsigned long  ulong;
typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef float float2[2];
typedef float float3[3];
typedef float float4[4];

struct nFloat4
{
    float x;
    float y;
    float z;
    float w;
};

struct nFloat3
{
    float x;
    float y;
    float z;
};

struct nFloat2
{
    float x;
    float y;
};
/**
    An nFourCC is usually represented in textual form in the source code
    as a character constant comprised of 4 characters:

    @verbatim
    'HELO'@endverbatim

    These four characters are interpreted as an integer value, making
    comparisons much faster than using a string identifier, while
    retaining, roughly, the readability of a string.

    These are used in the script interface when defining a script
    command via nClass::AddCmd(), as well as identifiers in the
    nVariableServer (and related nVariableContext and nVariable)
    classes.
*/
typedef unsigned int nFourCC;
typedef double nTime;

#ifndef NULL
#define NULL (0L)
#endif

//------------------------------------------------------------------------------
#define N_MAXPATH (512)     // maximum length for complete path
#define N_MAXNAMELEN (128)   // maximum length for single path component

//------------------------------------------------------------------------------
#define nID(a, b, c, d) ((a << 24) | (b << 16) | (c << 8) | (d))
#define MAKE_FOURCC(c0, c1, c2, c3) ((c0) | (c1 << 8) | (c2 << 16) | (c3 << 24))
#define FOURCC(i) (((i & 0xff000000) >> 24) | ((i & 0x00ff0000) >> 8) | ((i & 0x0000ff00) << 8) | ((i & 0x000000ff) << 24))
#define N_WHITESPACE " \r\n\t"

#if defined(__LINUX__) || defined(__MACOSX__)
#define n_stricmp strcasecmp
#else
#define n_stricmp stricmp
#endif

#ifdef __WIN32__
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif

#ifdef _MSC_VER
#define va_copy(d, s) d = s
#endif

#ifdef __GNUC__
// Hey! Look! A cute GNU C++ extension!
#define min(a, b)   a <? b
#define max(a, b)   a >? b
#endif

// maps unsigned 8 bits/channel to D3DCOLOR
#define N_ARGB(a, r, g, b) ((uint)((((a)&0xff) << 24) | (((r)&0xff) << 16) | (((g)&0xff) << 8) | ((b)&0xff)))
#define N_RGBA(r, g, b, a) N_ARGB(a, r, g, b)
#define N_XRGB(r, g, b)   N_ARGB(0xff, r, g, b)
#define N_COLORVALUE(r, g, b, a) N_RGBA((uint)((r)*255.f), (uint)((g)*255.f), (uint)((b)*255.f), (uint)((a)*255.f))

//------------------------------------------------------------------------------
//  public kernel C functions
//------------------------------------------------------------------------------
void __cdecl n_printf(const char*, ...)
    __attribute__((format(printf, 1, 2)));
void __cdecl n_error(const char*, ...)
    __attribute__((noreturn))
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

void n_barf(const char*, const char*, int)
    __attribute__((noreturn));
void n_barf2(const char*, const char*, const char*, int)
    __attribute__((noreturn));

nFourCC n_strtofourcc(const char*);
const char* n_fourcctostr(nFourCC);

//------------------------------------------------------------------------------
//  Nebula memory management and debugging stuff.
//------------------------------------------------------------------------------
extern bool nMemoryLoggingEnabled;
struct nMemoryStats
{
    int highWaterSize;      // max allocated size so far
    int totalCount;         // total number of allocations
    int totalSize;          // current allocated size
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

// implemented in ndbgalloc.cc
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

#define nNebulaUsePackage(PACKAGE) extern "C" void PACKAGE()

#define nNebulaClass(CLASS, SUPERCLASSNAME) \
    extern bool n_init(nClass* clazz, nKernelServer* kernelServer); \
    extern void* n_create(); \
    bool n_init(nClass* clazz, nKernelServer* kernelServer) {\
        clazz->SetProperName(#CLASS); \
		nKernelServer::Instance()->AddClass(SUPERCLASSNAME, clazz); \
        return true; \
    } \
    void* n_create() { return n_new(CLASS()); }

#define nNebulaRootClass(CLASS) \
    extern bool n_init(nClass* clazz, nKernelServer* kernelServer); \
    extern void* n_create(); \
    bool n_init(nClass* clazz, nKernelServer* kernelServer) {\
        clazz->SetProperName(#CLASS); \
        return true; \
    } \
    void* n_create() { return n_new(CLASS()); }

//--------------------------------------------------------------------
#endif
