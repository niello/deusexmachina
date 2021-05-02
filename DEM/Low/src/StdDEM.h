#pragma once
#include "StdCfg.h"
#include <System/Memory.h>
#include <stdint.h>
#include <vector>
#include <set>
#include <functional>
#include <algorithm>

#define OK				return true
#define FAIL			return false

constexpr auto INVALID_INDEX = ~static_cast<size_t>(0);

// http://cnicholson.net/2011/01/stupid-c-tricks-a-better-sizeof_array
template<typename T, size_t N> char (&SIZEOF_ARRAY_REQUIRES_ARRAY_ARGUMENT(T (&)[N]))[N];  
#define sizeof_array(x) sizeof(SIZEOF_ARRAY_REQUIRES_ARRAY_ARGUMENT(x))

#if defined(_MSC_VER) // __FUNCTION__ ## "()"
#   define DEM_FUNCTION_NAME __FUNCSIG__
#elif defined(__GNUC__)
#   define DEM_FUNCTION_NAME __PRETTY_FUNCTION__
#elif __STDC_VERSION__ >= 199901L
#   define DEM_FUNCTION_NAME __func__
#else
#   define DEM_FUNCTION_NAME "<DEM_FUNCTION_NAME>"
#endif

//---------------------------------------------------------------------
//  Shortcut typedefs
//---------------------------------------------------------------------

// New types

typedef intptr_t			IPTR;	// Signed integer of a pointer size
typedef uintptr_t			UPTR;	// Unsigned integer of a pointer size
typedef int8_t				I8;		// Signed 8-bit integer
typedef uint8_t				U8;		// Unsigned 8-bit integer
typedef int16_t				I16;	// Signed 16-bit integer
typedef uint16_t			U16;	// Unsigned 16-bit integer
typedef int32_t				I32;	// Signed 32-bit integer
typedef uint32_t			U32;	// Unsigned 32-bit integer
typedef int64_t				I64;	// Signed 64-bit integer
typedef uint64_t			U64;	// Unsigned 64-bit integer

typedef void*				PVOID;
typedef double				CTime;

const UPTR HalfRegisterBits	= sizeof(UPTR) << 2; // Bytes * 8 / 2
const UPTR INVALID_HALF_INDEX = (1 << HalfRegisterBits) - 1;

#define UPTR_LOW_HALF_MASK	INVALID_HALF_INDEX
#define UPTR_LOW_HALF(x)	((x) & UPTR_LOW_HALF_MASK)
#define UPTR_HIGH_HALF(x)	((x) >> HalfRegisterBits)

typedef UPTR				HHandle;

//---------------------------------------------------------------------
//  Compiler-dependent aliases
//---------------------------------------------------------------------
#if defined(__LINUX__) || defined(__MACOSX__)
#define n_stricmp strcasecmp
#else
#define n_stricmp _stricmp
#endif

#ifdef _MSC_VER
    #define DEM_FORCE_INLINE __forceinline
#elif defined(__GNUC__)
    #define DEM_FORCE_INLINE inline __attribute__((__always_inline__))
#elif defined(__CLANG__)
    #if __has_attribute(__always_inline__)
        #define DEM_FORCE_INLINE inline __attribute__((__always_inline__))
    #else
        #define DEM_FORCE_INLINE inline
    #endif
#else
    #define DEM_FORCE_INLINE inline
#endif

//---------------------------------------------------------------------
// Template magic not in std (yet?)
//---------------------------------------------------------------------

template<class T>
struct just_type
{
	using type = std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<T>>>;
};

template<class T> using just_type_t = typename just_type<T>::type;

template<class T>
struct ensure_pointer
{
	using type = std::remove_reference_t<std::remove_pointer_t<T>>*;
};

template<class T> using ensure_pointer_t = typename ensure_pointer<T>::type;

template <typename T, std::size_t ... Indices>
auto tuple_pop_front_impl(const T& tuple, std::index_sequence<Indices...>)
{
	return std::make_tuple(std::get<1 + Indices>(tuple)...);
}

template <typename T>
auto tuple_pop_front(const T& tuple)
{
	return tuple_pop_front_impl(tuple, std::make_index_sequence<std::tuple_size<T>::value - 1>());
}

//---------------------------------------------------------------------
//  Kernel and aux functions and enums
//---------------------------------------------------------------------
inline bool IsPow2(unsigned int Value) { return Value > 0 && (Value & (Value - 1)) == 0; }

template <class T>
inline T NextPow2(T x)
{
	if constexpr (std::is_signed_v<T>)
	{
		if (x < 0) return 0;
	}

	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;

	if constexpr (sizeof(T) > 4)
		x |= x >> 32;

	return x + 1;
}

// Only for power-of-2 alignment //!!!C++11 static_assert may help!
template <unsigned int Alignment> inline bool IsAligned(const void* Pointer) { return !(((unsigned int)Pointer) & (Alignment - 1)); }
inline bool IsAligned16(const void* Pointer) { return !(((unsigned int)Pointer) & 0x0000000f); }

template<typename T>
void VectorFastErase(std::vector<T>& Self, UPTR Index)
{
	if (Index >= Self.size()) return;
	if (Index < Self.size() - 1) std::swap(Self[Index], Self[Self.size() - 1]);
	Self.pop_back();
}

// Execution results

const UPTR Failure = 0;
const UPTR Success = 1;
const UPTR Running = 2;
const UPTR Error = 3;
// Use values bigger than Error to specify errors

inline bool ExecResultIsError(UPTR Result) { return Result >= Error; }

//

enum class ESoftBool : U8
{
	False = 0,
	True = 1,
	Maybe = 2
};

enum class EClipStatus
{
	Outside,
	Inside,
	Clipped,
	//InvalidClipStatus - Clipped is used instead now
};

struct CRational
{
	I32	Numerator;
	U32	Denominator;

	I32		GetIntRounded() const { return Denominator ? Numerator / Denominator : 0; }
	float	GetFloat() const { return Denominator ? (float)Numerator / (float)Denominator : 0.f; }
	double	GetDouble() const { return Denominator ? (double)Numerator / (double)Denominator : 0.0; }

	bool operator ==(const CRational& Other) const { return Numerator == Other.Numerator && Denominator == Other.Denominator; }
	bool operator !=(const CRational& Other) const { return Numerator != Other.Numerator || Denominator != Other.Denominator; }
};
