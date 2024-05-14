#pragma once
#include <Data/Metadata.h>
#include <Character/ModifiableParameter.h> // FIXME: not character but stats?
#include <Combat/Damage.h>

// Weapon is typically an item component which makes it capable of attacking targets and inflicting damage

namespace DEM::RPG
{

struct CDamageData
{
	EDamageType Type = EDamageType::Bludgeoning;
	uint8_t     x = 1;
	uint8_t     y = 1;
	int8_t      z = 0;
};

struct CWeaponComponent
{
	//!!!DBG TMP!
	CDamageData Damage;
	float       Range = 1.f;   // World units (meters)
	float       Period = 1.f;  // Seconds

	bool Big = false;
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::RPG::CDamageData>() { return "DEM::RPG::CDamageData"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CDamageData>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CDamageData, 1, Type),
		DEM_META_MEMBER_FIELD(RPG::CDamageData, 2, x),
		DEM_META_MEMBER_FIELD(RPG::CDamageData, 3, y),
		DEM_META_MEMBER_FIELD(RPG::CDamageData, 4, z)
	);
}

template<> inline constexpr auto RegisterClassName<DEM::RPG::CWeaponComponent>() { return "DEM::RPG::CWeaponComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CWeaponComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CWeaponComponent, 1, Damage),
		DEM_META_MEMBER_FIELD(RPG::CWeaponComponent, 2, Range),
		DEM_META_MEMBER_FIELD(RPG::CWeaponComponent, 3, Period),
		DEM_META_MEMBER_FIELD(RPG::CWeaponComponent, 4, Big)
	);
}

}
