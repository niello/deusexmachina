#pragma once
#include <Data/StringID.h>
#include <StdDEM.h>
#include <map>

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

	COUNT
};
constexpr size_t DamageTypeCount = static_cast<size_t>(EDamageType::COUNT);

class CDamageAbsorption : public std::map<CStrID, std::array<int, DamageTypeCount>> {};

}

