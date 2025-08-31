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

	Cheat
};
constexpr size_t DamageTypeCount = static_cast<size_t>(EDamageType::COUNT);

using CZoneDamageAbsorption = std::array<CNumericStat, DamageTypeCount>;

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

namespace DEM::Serialization
{

//!!!FIXME: need generic serialization for all enums!
template<>
struct ParamsFormat<DEM::RPG::EDamageType>
{
	static inline void Serialize(Data::CData& Output, DEM::RPG::EDamageType Value)
	{
		switch (Value)
		{
			case DEM::RPG::EDamageType::Piercing: Output = std::string("Piercing"); return;
			case DEM::RPG::EDamageType::Slashing: Output = std::string("Slashing"); return;
			case DEM::RPG::EDamageType::Bludgeoning: Output = std::string("Bludgeoning"); return;
			case DEM::RPG::EDamageType::Energetic: Output = std::string("Energetic"); return;
			case DEM::RPG::EDamageType::Chemical: Output = std::string("Chemical"); return;
			default: Output = {}; return;
		}
	}

	static inline void Deserialize(const Data::CData& Input, DEM::RPG::EDamageType& Value)
	{
		if (!Input.IsA<std::string>()) return;

		const std::string_view InputStr = Input.GetValue<std::string>();
		if (InputStr == "Piercing") Value = DEM::RPG::EDamageType::Piercing;
		else if (InputStr == "Slashing") Value = DEM::RPG::EDamageType::Slashing;
		else if (InputStr == "Bludgeoning") Value = DEM::RPG::EDamageType::Bludgeoning;
		else if (InputStr == "Energetic") Value = DEM::RPG::EDamageType::Energetic;
		else if (InputStr == "Chemical") Value = DEM::RPG::EDamageType::Chemical;
	}
};

}

