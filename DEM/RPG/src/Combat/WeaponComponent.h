#pragma once
#include <Combat/Damage.h>
#include <Scripting/CommandList.h>

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
	//!!!TODO: damage as a command in a list? will be hard to show stats and precalc damage?
	CDamageData        Damage;
	Game::CCommandList OnHit;
	float              Range = 1.f;   // World units (meters)
	float              Period = 1.f;  // Seconds
	bool               Big = false;
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<RPG::CDamageData>() { return "DEM::RPG::CDamageData"; }
template<> constexpr auto RegisterMembers<RPG::CDamageData>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CDamageData, Type),
		DEM_META_MEMBER_FIELD(RPG::CDamageData, x),
		DEM_META_MEMBER_FIELD(RPG::CDamageData, y),
		DEM_META_MEMBER_FIELD(RPG::CDamageData, z)
	);
}
static_assert(CMetadata<RPG::CDamageData>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

template<> constexpr auto RegisterClassName<RPG::CWeaponComponent>() { return "DEM::RPG::CWeaponComponent"; }
template<> constexpr auto RegisterMembers<RPG::CWeaponComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CWeaponComponent, Damage),
		DEM_META_MEMBER_FIELD(RPG::CWeaponComponent, OnHit),
		DEM_META_MEMBER_FIELD(RPG::CWeaponComponent, Range),
		DEM_META_MEMBER_FIELD(RPG::CWeaponComponent, Period),
		DEM_META_MEMBER_FIELD(RPG::CWeaponComponent, Big)
	);
}
static_assert(CMetadata<RPG::CWeaponComponent>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

}
