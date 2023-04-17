#pragma once
#include "StdCfg.h"
#include <System/Memory.h>
#include <stdint.h>
#include <vector>
#include <set>
#include <functional>
#include <algorithm>
#if __cplusplus >= 202002L
#include <bit>
#endif

#define OK   return true
#define FAIL return false

template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
constexpr T INVALID_INDEX_T = ~static_cast<T>(0);

constexpr size_t INVALID_INDEX = ~static_cast<size_t>(0);

template<typename T, size_t N>
constexpr size_t sizeof_array(const T(&)[N]) { return N; }

#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
#define DEM_X64 (1)
#else
#define DEM_X86 (1)
#endif

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

// Only for power-of-2 alignment //!!!C++11 static_assert may help!
template <unsigned int Alignment> DEM_FORCE_INLINE bool IsAligned(const void* Pointer) { return !(((unsigned int)Pointer) & (Alignment - 1)); }
DEM_FORCE_INLINE bool IsAligned16(const void* Pointer) { return !(((unsigned int)Pointer) & 0x0000000f); }

template<typename T>
DEM_FORCE_INLINE bool VectorFastErase(std::vector<T>& Self, UPTR Index)
{
	if (Index >= Self.size()) return false;
	if (Index < Self.size() - 1) std::swap(Self[Index], Self[Self.size() - 1]);
	Self.pop_back();
	return true;
}

template<typename T>
DEM_FORCE_INLINE bool VectorFastErase(std::vector<T>& Self, typename std::vector<T>::iterator It)
{
	if (It == Self.cend()) return false;
	if (It != --Self.cend())
		std::swap(*It, Self.back());
	Self.pop_back();
	return true;
}

template<typename T>
DEM_FORCE_INLINE bool VectorFastErase(std::vector<T>& Self, const T& Value)
{
	return VectorFastErase(Self, std::find(Self.begin(), Self.end(), Value));
}

// Execution results

const UPTR Failure = 0;
const UPTR Success = 1;
const UPTR Running = 2;
const UPTR Error = 3;
// Use values bigger than Error to specify errors

DEM_FORCE_INLINE bool ExecResultIsError(UPTR Result) { return Result >= Error; }

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
