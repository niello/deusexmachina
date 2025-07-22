#pragma once
#include "StdCfg.h"
#include <System/Memory.h>
#include <tracy/Tracy.hpp>
#include <fmt/format.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <set>
#include <functional>
#include <algorithm>

using namespace std::string_view_literals;
using namespace fmt::literals;
auto operator"" _format(const char* s, size_t n) {
	return [=](auto&&... args) {
		return fmt::format(fmt::runtime(std::string_view(s, n)), args...);
	};
}

inline static const std::string EmptyString{};

#define OK   return true
#define FAIL return false

// Used for concatenating with predefined macros like __LINE__
#define CONCATENATE_INTERNAL(x, y) x ## y
#define CONCATENATE(x, y) CONCATENATE_INTERNAL(x, y)

template<typename T, typename = std::enable_if_t<std::is_integral_v<T> && !std::is_signed_v<T>>>
constexpr T INVALID_INDEX_T = ~static_cast<T>(0);

constexpr size_t INVALID_INDEX = ~static_cast<size_t>(0);

template<typename T, size_t N>
constexpr size_t sizeof_array(const T(&)[N]) { return N; }

// See https://sourceforge.net/p/predef/wiki/Architectures/
#if defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(__amd64__) || defined(__amd64) || defined(_M_AMD64)
#define DEM_CPU_ARCH_X86_64 (1)
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
#define DEM_CPU_ARCH_X86 (1)
#elif defined(__aarch64__) || defined(_M_ARM64)
#define DEM_CPU_ARCH_ARM64 (1)
#endif

#if DEM_CPU_ARCH_X86_64 || DEM_CPU_ARCH_ARM64
#define DEM_CPU_64 (1)
#else
#define DEM_CPU_32 (1)
#endif

#if DEM_CPU_ARCH_X86_64 || DEM_CPU_ARCH_X86
#define DEM_CPU_ARCH_X86_COMPATIBLE (1)
#define DEM_CPU_REORDER_LOAD_LOAD (0)
#define DEM_CPU_REORDER_LOAD_STORE (0)
#define DEM_CPU_REORDER_STORE_LOAD (1)
#define DEM_CPU_REORDER_STORE_STORE (0)
#else
// Imagine the worst case by default
#define DEM_CPU_REORDER_LOAD_LOAD (1)
#define DEM_CPU_REORDER_LOAD_STORE (1)
#define DEM_CPU_REORDER_STORE_LOAD (1)
#define DEM_CPU_REORDER_STORE_STORE (1)
#endif

#if defined(__BMI2__) || defined(__AVX2__)
#define DEM_BMI2 (1)
#endif

#if defined(__AVX2__)
#define DEM_F16C (1)
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

#ifdef _MSC_VER
    #define DEM_NO_INLINE __declspec(noinline)
#elif defined(__GNUC__) ||  defined(__CLANG__)
	#define DEM_FORCE_INLINE __attribute__((noinline))
#else
    #define DEM_NO_INLINE
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

template<typename... T>
DEM_FORCE_INLINE constexpr decltype(auto) ENUM_MASK(T... Values)
{
	// TODO: support enum class with std::underlying_type_t. Maybe write 2 overloads with enable_if?
	return ((1 << Values) | ...);
}
//---------------------------------------------------------------------

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

enum EClipStatus
{
	Inside = 0x01,
	Outside = 0x02,
	Clipped = (Outside | Inside),
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
