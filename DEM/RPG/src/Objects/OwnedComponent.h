#pragma once
#include <Game/ECS/Entity.h>

// Makes an entity belong to some other entity (owner) or a faction

namespace DEM::RPG
{

struct COwnedComponent
{
	Game::HEntity Owner;
	//TODO: faction ID, use if no owner?
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::RPG::COwnedComponent>() { return "DEM::RPG::COwnedComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::COwnedComponent>()
{
	return std::make_tuple
	(
		Member(1, "Owner", &DEM::RPG::COwnedComponent::Owner, &DEM::RPG::COwnedComponent::Owner)
	);
}

}
