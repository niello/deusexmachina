#pragma once
#include <Data/Metadata.h>
#include <Combat/Damage.h>

// Armor stats for an item

namespace DEM::RPG
{

struct CArmorComponent
{
	// Need to apply/remove modifiers for equipped instances
	static constexpr bool Signals = true;

	std::map<CStrID, CZoneDamageAbsorptionMod> Absorption;
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<DEM::RPG::CArmorComponent>() { return "DEM::RPG::CArmorComponent"; }
template<> constexpr auto RegisterMembers<DEM::RPG::CArmorComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CArmorComponent, Absorption)
	);
}

}
