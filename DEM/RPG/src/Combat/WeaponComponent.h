#pragma once
#include <Data/Metadata.h>
#include <Character/ModifiableParameter.h> // FIXME: not character but stats?
#include <Combat/Damage.h>

// Weapon is typically an item component which makes it capable of attacking targets and inflicting damage

namespace DEM::RPG
{

struct CWeaponComponent
{
	bool Big = false;
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::RPG::CWeaponComponent>() { return "DEM::RPG::CWeaponComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CWeaponComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CWeaponComponent, 1, Big)
	);
}

}
