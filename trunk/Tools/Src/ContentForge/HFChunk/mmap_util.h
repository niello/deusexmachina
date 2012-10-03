// mmap_util.h	-- Thatcher Ulrich <tu@tulrich.com> 2002

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some utility functions for dealing with memory mapping using mmap
// (Posix) or MapViewOfFile (Win32).


#ifndef MMAP_UTIL_H
#define MMAP_UTIL_H


// These functions wrap platform-specific code.
namespace mmap_util {
	void*	map(int size, bool writable, const char* filename);
	void	unmap(void* buffer, int size);
};


#endif // MMAP_UTIL_H
