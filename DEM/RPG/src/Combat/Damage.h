#pragma once
#include <Data/StringID.h>
#include <StdDEM.h>
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

	COUNT
};
constexpr size_t DamageTypeCount = static_cast<size_t>(EDamageType::COUNT);

// HitZone -> AbsorptionValue[DamageType]
class CDamageAbsorption : public std::map<CStrID, std::array<int, DamageTypeCount>>
{
public:

	CDamageAbsorption& operator +=(const CDamageAbsorption& Other)
	{
		for (const auto& [Zone, Absorption] : Other)
		{
			auto It = find(Zone);
			if (It == cend())
			{
				emplace(Zone, Absorption);
			}
			else
			{
				for (size_t i = 0; i < Absorption.size(); ++i)
					It->second[i] += Absorption[i];
			}
		}

		return *this;
	}
};

}

