#pragma once
#include <Data/Metadata.h>
#include <Data/FixedOrderMap.h>
#include <StdDEM.h>

// Generic equippable settings. They override implicit equipment rules like 'all weapons can be equipped to hands'.

namespace DEM::RPG
{

struct CEquippableComponent
{
	CStrID                            ScriptAssetID;
	std::vector<Resources::PResource> AppearanceAssets;
	CFixedOrderMap<CStrID, U32>       Slots;        // A list of slot types blocked by this entity when equipped, with count for each type
	U32                               MaxStack = 1; // Maximum count of these items in the equipment slot
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::RPG::CEquippableComponent>() { return "DEM::RPG::CEquippableComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CEquippableComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CEquippableComponent, Slots),
		DEM_META_MEMBER_FIELD(RPG::CEquippableComponent, MaxStack),
		DEM_META_MEMBER_FIELD(RPG::CEquippableComponent, ScriptAssetID),
		DEM_META_MEMBER_FIELD(RPG::CEquippableComponent, AppearanceAssets)
	);
}

}
