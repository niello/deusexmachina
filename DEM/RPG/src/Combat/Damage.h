#pragma once
#include <Data/StringID.h>
#include <StdDEM.h>
#include <map>

// An entity with a destructible component will be logically destroyed (killed) when it runs out of HP

namespace DEM::RPG
{

enum class EDamageType : U8
{
	Piercing = 0,
	Slashing,
	Bludgeoning,
	Energetic,
	Chemical,

	COUNT
};
constexpr size_t DamageTypeCount = static_cast<size_t>(EDamageType::COUNT);

//using CDamageAbsorption = std::map<CStrID, int[DamageTypeCount]>;
class CDamageAbsorption : public std::map<CStrID, int[DamageTypeCount]> {};

}

