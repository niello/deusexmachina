#pragma once
#include <Game/ECS/Entity.h>

// Makes an entity belong to other entity (normally character or creature)
// and/or to factions. For faction membership use CSocialComponent.

namespace DEM::RPG
{

struct COwnedComponent
{
	Game::HEntity    Owner;
	std::set<CStrID> Factions;
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<RPG::COwnedComponent>() { return "DEM::RPG::COwnedComponent"; }
template<> constexpr auto RegisterMembers<RPG::COwnedComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::COwnedComponent, Owner),
		DEM_META_MEMBER_FIELD(RPG::COwnedComponent, Factions)
	);
}

}
