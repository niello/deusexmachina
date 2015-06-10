#pragma once
#ifndef __DEM_L1_MEMORY_H__
#define __DEM_L1_MEMORY_H__

#include <stdlib.h>
#include <new>

// Memory management overrides

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
void* n_malloc_aligned_dbg(size_t size, size_t Alignment, const char* filename, int line);
void* n_calloc_dbg(size_t num, size_t size, const char* file, int line);
void* n_realloc_dbg(void* memblock, size_t size, const char* file, int line);
void* n_realloc_aligned_dbg(void* memblock, size_t size, size_t Alignment, const char* filename, int line);
void n_free_dbg(void* memblock, const char* file, int line);
void n_free_aligned_dbg(void* memblock, const char* filename, int line);

#if defined(_DEBUG) && defined(__WIN32__)
#define n_new(type) new(__FILE__,__LINE__) type
#define n_placement_new(place, type) new(place, __FILE__,__LINE__) type
#define n_new_array(type,size) new(__FILE__,__LINE__) type[size]
#define n_delete(ptr) delete ptr
#define n_delete_array(ptr) delete[] ptr
#define n_malloc(size) n_malloc_dbg(size, __FILE__, __LINE__)
#define n_malloc_aligned(size, alignment) n_malloc_aligned_dbg(size, alignment, __FILE__, __LINE__)
#define n_calloc(num, size) n_calloc_dbg(num, size, __FILE__, __LINE__)
#define n_realloc(memblock, size) n_realloc_dbg(memblock, size, __FILE__, __LINE__)
#define n_realloc_aligned(memblock, size, alignment) n_realloc_dbg(memblock, size, alignment, __FILE__, __LINE__)
#define n_free(memblock) n_free_dbg(memblock, __FILE__, __LINE__)
#define n_free_aligned(memblock) n_free_aligned_dbg(memblock, __FILE__, __LINE__)
#else
#define n_new(type) new type
#define n_placement_new(place, type) new(place) type
#define n_new_array(type,size) new type[size]
#define n_delete(ptr) delete ptr
#define n_delete_array(ptr) delete[] ptr
#define n_malloc(size) malloc(size)
#define n_malloc_aligned(size, alignment) _aligned_malloc(size, alignment)
#define n_calloc(num, size) calloc(num, size)
#define n_realloc(memblock, size) realloc(memblock, size)
#define n_realloc_aligned(memblock, size, alignment) _aligned_realloc(memblock, size, alignment)
#define n_free(memblock) free(memblock)
#define n_free_aligned(memblock) _aligned_free(memblock)
#endif

#define SAFE_RELEASE(n)			if (n) { n->Release(); n = NULL; }
#define SAFE_DELETE(n)			if (n) { n_delete(n); n = NULL; }
#define SAFE_DELETE_ARRAY(n)	if (n) { n_delete_array(n); n = NULL; }
#define SAFE_FREE(n)			if (n) { n_free(n); n = NULL; }
#define SAFE_FREE_ALIGNED(n)	if (n) { n_free_aligned(n); n = NULL; }

#if defined(_MSC_VER)
#define DEM_ALIGN_16 __declspec(align(16))
#else
#error "Please define align 16 for your compiler"
#endif

#define DEM_ALLOCATE_ALIGN16 \
   inline void* operator new(size_t sizeInBytes)	{ return n_malloc_aligned(sizeInBytes, 16); }   \
   inline void  operator delete(void* ptr)			{ n_free_aligned(ptr); }   \
   inline void* operator new(size_t, void* ptr)		{ return ptr; }   \
   inline void  operator delete(void*, void*)		{ }   \
   inline void* operator new[](size_t sizeInBytes)	{ n_malloc_aligned(sizeInBytes, 16); }   \
   inline void  operator delete[](void* ptr)		{ n_free_aligned(ptr); }   \
   inline void* operator new[](size_t, void* ptr)	{ return ptr; }   \
   inline void  operator delete[](void*, void*)		{ }   \
   inline void* operator new(size_t sizeInBytes, const char* file, int line)	{ return n_malloc_aligned(sizeInBytes, 16); }   \
   inline void  operator delete(void* ptr, const char* file, int line)			{ n_free_aligned(ptr); }   \
   inline void* operator new(size_t, void* ptr, const char* file, int line)		{ return ptr; }   \
   inline void  operator delete(void*, void*, const char* file, int line)		{ }   \
   inline void* operator new[](size_t sizeInBytes, const char* file, int line)	{ n_malloc_aligned(sizeInBytes, 16); }   \
   inline void  operator delete[](void* ptr, const char* file, int line)		{ n_free_aligned(ptr); }   \
   inline void* operator new[](size_t, void* ptr, const char* file, int line)	{ return ptr; }   \
   inline void  operator delete[](void*, void*, const char* file, int line)		{ }   \

#endif