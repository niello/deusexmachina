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

template<> constexpr auto RegisterClassName<DEM::RPG::CDamageData>() { return "DEM::RPG::CDamageData"; }
template<> constexpr auto RegisterMembers<DEM::RPG::CDamageData>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CDamageData, Type),
		DEM_META_MEMBER_FIELD(RPG::CDamageData, x),
		DEM_META_MEMBER_FIELD(RPG::CDamageData, y),
		DEM_META_MEMBER_FIELD(RPG::CDamageData, z)
	);
}

template<> constexpr auto RegisterClassName<DEM::RPG::CWeaponComponent>() { return "DEM::RPG::CWeaponComponent"; }
template<> constexpr auto RegisterMembers<DEM::RPG::CWeaponComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CWeaponComponent, Damage),
		DEM_META_MEMBER_FIELD(RPG::CWeaponComponent, Range),
		DEM_META_MEMBER_FIELD(RPG::CWeaponComponent, Period),
		DEM_META_MEMBER_FIELD(RPG::CWeaponComponent, Big)
	);
}

}
