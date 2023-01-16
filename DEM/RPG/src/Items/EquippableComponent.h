#pragma once
#include <Data/Metadata.h>
#include <Data/FixedOrderMap.h>
#include <StdDEM.h>

// Generic equippable settings. They override implicit equipment rules like 'all weapons can be equipped to hands'.

namespace DEM::RPG
{

struct CEquippableComponent
{
	CFixedOrderMap<CStrID, U32> Slots;        // A list of slot types blocked by this entity when equipped, with count for each type
	//std::map<CStrID, U32> Slots;        // A list of slot types blocked by this entity when equipped, with count for each type
	U32                   MaxStack = 1; // Maximum count of these items in the equipment slot
	//???bool TryScript?
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::RPG::CEquippableComponent>() { return "DEM::RPG::CEquippableComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CEquippableComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CEquippableComponent, 1, Slots),
		DEM_META_MEMBER_FIELD(RPG::CEquippableComponent, 2, MaxStack)
	);
}

}
