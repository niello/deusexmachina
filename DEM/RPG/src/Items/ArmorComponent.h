#pragma once
#include <Data/Metadata.h>
#include <StdDEM.h>
#include <map>

// Armor stats for an item

namespace DEM::RPG
{

struct CArmorComponent
{
	std::map<CStrID, int[5]> Absorption; // Zone -> AbsorptionValue[DamageType]
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::RPG::CArmorComponent>() { return "DEM::RPG::CArmorComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CArmorComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CArmorComponent, 1, Absorption)
	);
}

}
