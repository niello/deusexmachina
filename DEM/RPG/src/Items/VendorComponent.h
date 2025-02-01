#pragma once
#include <Resources/Resource.h>
#include <Game/ECS/Entity.h>
#include <Data/Metadata.h>

// Allows an item to serve as a lockpick

namespace DEM::RPG
{

struct CVendorComponent
{
	Game::HEntity        ContainerID;
	Resources::PResource ItemGeneratorAsset; // CItemList
	// last gen timestamp in world time, default 0
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<RPG::CVendorComponent>() { return "DEM::RPG::CVendorComponent"; }
template<> constexpr auto RegisterMembers<RPG::CVendorComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CVendorComponent, ContainerID),
		DEM_META_MEMBER_FIELD(RPG::CVendorComponent, ItemGeneratorAsset)
	);
}
static_assert(CMetadata<RPG::CVendorComponent>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

}
