// utility.cpp	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Various little utility functions, macros & typedefs.


#include "engine/utility.h"
#include "engine/dlmalloc.h"


#ifdef _WIN32
#ifndef NDEBUG

int	tu_testbed_assert_break(const char* filename, int linenum, const char* expression)
{
	// @@ TODO output print error message
	__asm { int 3 }
	return 0;
}

#endif // not NDEBUG
#endif // _WIN32


#ifdef USE_DL_MALLOC

// Overrides of new/delete that use Doug Lea's malloc.  Very helpful
// on certain lame platforms.

void*	operator new(size_t size)
{
	return dlmalloc(size);
}

void	operator delete(void* ptr)
{
	if (ptr) dlfree(ptr);
}

void*	operator new[](size_t size)
{
	return dlmalloc(size);
}

void	operator delete[](void* ptr)
{
	if (ptr) dlfree(ptr);
}

#endif // USE_DL_MALLOC
