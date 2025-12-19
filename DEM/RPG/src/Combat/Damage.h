#pragma once
#include <Character/NumericStat.h>
#include <map>
#include <array>

// Data structures and algorithms related to damage infliction

namespace DEM::RPG
{

//???ise 1<<x for masks or constexpr log2 for indexing?
//???use enum_count, enum_index and constexpr |-aliases for mask matching?
//!!!there is enum_flags_cast for strings!

enum class EDamageType : U8
{
	Piercing = 0, // 1 << 0
	Slashing,
	Bludgeoning,
	Energetic,
	Chemical,

	COUNT,

	Raw // Direct non-absorbable damage
};
//template <>
//struct magic_enum::customize::enum_range<Directions>
//{
//	static constexpr bool is_flags = true;
//};
constexpr size_t DamageTypeCount = static_cast<size_t>(EDamageType::COUNT);

static inline bool IsAbsorbableDamageType(EDamageType Type) { return Type >= EDamageType::Piercing && Type < EDamageType::COUNT; }
static inline bool IsPhysicalDamageType(EDamageType Type) { return Type >= EDamageType::Piercing && Type <= EDamageType::Bludgeoning; }

using CZoneDamageAbsorptionStat = std::array<CNumericStat, DamageTypeCount>;
using CZoneDamageAbsorptionMod = std::array<int, DamageTypeCount>;

}
