#pragma once
#include <Data/Metadata.h>

// Makes an entity able to contain items

namespace DEM::RPG
{

struct CItemContainerComponent
{
	std::vector<Game::HEntity> Items;
	float                      MaxVolume = -1.f;
	bool                       Temporary = false; // Destroy entity when becomes empty
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<RPG::CItemContainerComponent>() { return "DEM::RPG::CItemContainerComponent"; }
template<> constexpr auto RegisterMembers<RPG::CItemContainerComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CItemContainerComponent, Items),
		DEM_META_MEMBER_FIELD(RPG::CItemContainerComponent, MaxVolume),
		DEM_META_MEMBER_FIELD(RPG::CItemContainerComponent, Temporary)
	);
}

}
