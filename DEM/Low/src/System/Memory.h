#pragma once
#include <stdlib.h>
#include <new>
#include <memory>

// Memory management overrides

extern bool DEM_LogMemory;
struct CMemoryStats
{
	int HighWaterSize;      // max allocated size so far
	int TotalCount;         // total number of allocations
	int TotalSize;          // current allocated size
};

int n_dbgmemdumpleaks();
void n_dbgmeminit();                // initialize memory debugging system
CMemoryStats n_dbgmemgetstats();    // defined in ndbgalloc.cc

void* n_malloc_aligned_dbg(size_t size, size_t Alignment, const char* filename, int line);
void* n_realloc_aligned_dbg(void* memblock, size_t size, size_t Alignment, const char* filename, int line);
void n_free_aligned_dbg(void* memblock, const char* filename, int line);

// FIXME: get rid of this
#define n_new(type) new type
#define n_placement_new(place, type) new(place) type
#define n_new_array(type,size) new type[size]
#define n_delete(ptr) delete ptr
#define n_delete_array(ptr) delete[] ptr

#if DEM_PLATFORM_WIN32
#if defined(_DEBUG)
#define n_malloc_aligned(size, alignment) n_malloc_aligned_dbg(size, alignment, __FILE__, __LINE__)
#define n_realloc_aligned(memblock, size, alignment) n_realloc_aligned_dbg(memblock, size, alignment, __FILE__, __LINE__)
#define n_free_aligned(memblock) n_free_aligned_dbg(memblock, __FILE__, __LINE__)
#else
#define n_malloc_aligned(size, alignment) _aligned_malloc(size, alignment)
#define n_realloc_aligned(memblock, size, alignment) _aligned_realloc(memblock, size, alignment)
#define n_free_aligned(memblock) _aligned_free(memblock)
#endif
#else
#define n_malloc_aligned(size, alignment) std::aligned_alloc(size, alignment)
#error "Need n_realloc_aligned"
//#define n_realloc_aligned(memblock, size, alignment) _aligned_realloc(memblock, size, alignment)
#define n_free_aligned(memblock) std::free(memblock)
#endif

struct CDeleterFree { void operator()(void* x) { std::free(x); } };
template<typename T> using unique_ptr_free = std::unique_ptr<T, CDeleterFree>;

struct CDeleterFreeAligned { void operator()(void* x) { n_free_aligned(x); } };
template<typename T> using unique_ptr_aligned = std::unique_ptr<T, CDeleterFreeAligned>;

#define SAFE_RELEASE(n)			if (n) { n->Release(); n = nullptr; }
#define SAFE_DELETE(n)			if (n) { n_delete(n); n = nullptr; }
#define SAFE_DELETE_ARRAY(n)	if (n) { n_delete_array(n); n = nullptr; }
#define SAFE_FREE(n)			if (n) { std::free(n); n = nullptr; }
#define SAFE_FREE_ALIGNED(n)	if (n) { n_free_aligned(n); n = nullptr; }

// NB: you have to declare a virtual destructor in a base class if your derived class is aligned and base is not!
#define DEM_ALLOCATE_ALIGNED(Alignment) \
   inline void* operator new(size_t sizeInBytes)	{ return n_malloc_aligned(sizeInBytes, Alignment); }   \
   inline void  operator delete(void* ptr)			{ n_free_aligned(ptr); }   \
   inline void* operator new(size_t, void* ptr)		{ return ptr; }   \
   inline void  operator delete(void*, void*)		{ }   \
   inline void* operator new[](size_t sizeInBytes)	{ return n_malloc_aligned(sizeInBytes, Alignment); }   \
   inline void  operator delete[](void* ptr)		{ n_free_aligned(ptr); }   \
   inline void* operator new[](size_t, void* ptr)	{ return ptr; }   \
   inline void  operator delete[](void*, void*)		{ }   \
   inline void* operator new(size_t sizeInBytes, const char* file, int line)	{ return n_malloc_aligned(sizeInBytes, Alignment); }   \
   inline void  operator delete(void* ptr, const char* file, int line)			{ n_free_aligned(ptr); }   \
   inline void* operator new(size_t, void* ptr, const char* file, int line)		{ return ptr; }   \
   inline void  operator delete(void*, void*, const char* file, int line)		{ }   \
   inline void* operator new[](size_t sizeInBytes, const char* file, int line)	{ return n_malloc_aligned(sizeInBytes, Alignment); }   \
   inline void  operator delete[](void* ptr, const char* file, int line)		{ n_free_aligned(ptr); }   \
   inline void* operator new[](size_t, void* ptr, const char* file, int line)	{ return ptr; }   \
   inline void  operator delete[](void*, void*, const char* file, int line)		{ }   \
