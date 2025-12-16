#pragma once
#include <Character/NumericStat.h>
#include <map>
#include <array>

// Data structures and algorithms related to damage infliction

namespace DEM::RPG
{

enum class EDamageType : U8
{
	Piercing = 0,
	Slashing,
	Bludgeoning,
	Energetic,
	Chemical,

	COUNT,

	Raw // Direct non-absorbable damage
};
constexpr size_t DamageTypeCount = static_cast<size_t>(EDamageType::COUNT);

static inline bool IsAbsorbableDamageType(EDamageType Type) { return Type >= EDamageType::Piercing && Type < EDamageType::COUNT; }
static inline bool IsPhysicalDamageType(EDamageType Type) { return Type >= EDamageType::Piercing && Type <= EDamageType::Bludgeoning; }

using CZoneDamageAbsorptionStat = std::array<CNumericStat, DamageTypeCount>;
using CZoneDamageAbsorptionMod = std::array<int, DamageTypeCount>;

}
