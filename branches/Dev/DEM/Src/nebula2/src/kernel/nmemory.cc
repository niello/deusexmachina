//------------------------------------------------------------------------------
/**
    @class nMemManager
    @ingroup NebulaKernelMemory

    Nebula memory management class. Optimizes memory allocations and tracks
    memory leaks and corruptions.

    (C) 1999 RadonLabs GmbH
*/
#include "kernel/ntypes.h"

#if defined(__NEBULA_MEM_MANAGER__) || defined(DOXYGEN)

// N2 memory manager was removed.
// dlmalloc to be used.

#elif __WIN32__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//------------------------------------------------------------------------------
/**
    Uses Windows memory management.

    @todo should these be updated to use heap allocations?
    http://msdn.microsoft.com/library/default.asp?url=/library/en-us/memory/base/heap_functions.asp
*/
void* nn_malloc(size_t size, const char* file, int line)
{
    HGLOBAL ptr = GlobalAlloc(0, size);
    n_assert(ptr);
    return ptr;
}

//------------------------------------------------------------------------------
/**
    Uses Windows memory management.
*/
void nn_free(void* p)
{
    GlobalFree((HGLOBAL) p);
}

//------------------------------------------------------------------------------
/**
    Uses Windows memory management.
*/
void* nn_calloc(size_t num, size_t size, const char*, int)
{
    HGLOBAL ptr = GlobalAlloc(GMEM_ZEROINIT, size*num);
    n_assert(ptr);
    return ptr;
}


//------------------------------------------------------------------------------
/**
    Uses Windows memory management.
*/
void* nn_realloc(void* p, size_t size, const char*, int)
{
    HGLOBAL ptr = GlobalReAlloc((HGLOBAL)p, size, GMEM_MOVEABLE);
    n_assert(ptr);
    return ptr;
}
//-------------------------------------------------------------------
#else

// Here was standart mem mgmt through malloc-realloc-free

#endif

//-------------------------------------------------------------------
//  EOF
//-------------------------------------------------------------------
