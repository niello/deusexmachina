#pragma once
#include <Data/CategorizationTraits.h>
#include <StdDEM.h>

// Utility algorithms

// A helper for incrementing source version. Guarantees that the version will newer be 0,
// so destination can use 0 as an out of sync state indicator for the forced update triggering.
template<typename T, typename std::enable_if_t<DEM::Meta::is_unsigned_integer_v<T>, void>* = nullptr>
DEM_FORCE_INLINE constexpr void IncrementVersion(T& Version) noexcept
{
	Version = std::max<T>(1, Version + 1);
}
//---------------------------------------------------------------------
