// utility.h	-- by Thatcher Ulrich <tu@tulrich.com>

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Various little utility functions, macros & typedefs.


#ifndef UTILITY_H
#define UTILITY_H

#include <assert.h>
#include <math.h>
#include <SDL.h>


#ifdef _WIN32
#ifndef NDEBUG

// On windows, replace ANSI assert with our own, for a less annoying
// debugging experience.
int	tu_testbed_assert_break(const char* filename, int linenum, const char* expression);
#undef assert
#define assert(x)	((x) || tu_testbed_assert_break(__FILE__, __LINE__, #x))

#endif // not NDEBUG
#endif // _WIN32


//
// new/delete wackiness -- if USE_DL_MALLOC is defined, we're going to
// try to use Doug Lea's malloc as much as possible by overriding the
// default operator new/delete.
//
#ifdef USE_DL_MALLOC

void*	operator new(size_t size);
void	operator delete(void* ptr);
void*	operator new[](size_t size);
void	operator delete[](void* ptr);

#else	// not USE_DL_MALLOC

// If we're not using DL_MALLOC, then *really* don't use it: #define
// away dlmalloc(), dlfree(), etc, back to the platform defaults.
#define dlmalloc	malloc
#define dlfree	free
#define dlrealloc	realloc
#define dlcalloc	calloc
#define dlmemalign	memalign
#define dlvalloc	valloc
#define dlpvalloc	pvalloc
#define dlmalloc_trim	malloc_trim
#define dlmalloc_stats	malloc_stats

#endif	// not USE_DL_MALLOC


#ifndef M_PI
#define M_PI 3.141592654
#endif // M_PI


//
// some misc handy math functions
//
inline int	iabs(int i) { if (i < 0) return -i; else return i; }
#ifdef __GNUC__
	// use the builtin (gcc) operator. ugly, but not my call.
	#define imax _max
	#define fmax _max
	#define _max(a,b) ((a)>?(b))
	#define imin _min
	#define fmin _min
	#define _min(a,b) ((a)<?(b))
#else // not GCC
	inline int	imax(int a, int b) { if (a < b) return b; else return a; }
	inline float	fmax(float a, float b) { if (a < b) return b; else return a; }
	inline int	imin(int a, int b) { if (a < b) return a; else return b; }
	inline float	fmin(float a, float b) { if (a < b) return a; else return b; }
#endif // not GCC

inline int	iclamp(int i, int min, int max) {
	assert( min <= max );
	return imax(min, imin(i, max));
}

inline float	fclamp(float f, float min, float max) {
	assert( min <= max );
	return fmax(min, fmin(f, max));
}

const float LN_2 = 0.693147180559945f;
inline float	log2(float f) { return logf(f) / LN_2; }

inline int	fchop( float f ) { return (int) f; }	// replace w/ inline asm if desired
inline int	frnd(float f) { return fchop(f + 0.5f); }	// replace with inline asm if desired


//
// Read/write bytes to SDL_RWops streams.
//


inline void	WriteByte(SDL_RWops* dst, Uint8 b) {
	dst->write(dst, &b, sizeof(b), 1);
}


inline Uint8	ReadByte(SDL_RWops* src) {
	Uint8	b;
	src->read(src, &b, sizeof(b), 1);	// @@ check for error
	return b;
}


//
// Read/write 32-bit little-endian floats, and 64-bit little-endian doubles.
//


inline void	WriteFloat32(SDL_RWops* dst, float value) {
	union {
		float	f;
		int	i;
	} u;
	u.f = value;
	SDL_WriteLE32(dst, u.i);
}


inline float	ReadFloat32(SDL_RWops* src) {
	union {
		float	f;
		int	i;
	} u;
	u.i = SDL_ReadLE32(src);
	return u.f;
}


inline void	WriteDouble64(SDL_RWops* dst, double value) {
	union {
		double	d;
		Uint64	l;
	} u;
	u.d = value;
	SDL_WriteLE64(dst, u.l);
}


inline double	ReadDouble64(SDL_RWops* src) {
	union {
		double	d;
		Uint64	l;
	} u;
	u.l = SDL_ReadLE64(src);
	return u.d;
}


#endif // UTILITY_H
