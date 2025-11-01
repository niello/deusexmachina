#pragma once
#include <Data/Metadata.h>
#include <Game/ECS/Entity.h>
#include <sol/sol.hpp>

// A component that marks individual item stacks as equipped and holds their modifiers applied to the equipment owner.
// The only reason to have modifiers here is to hold strong references. This component is not persistent.

namespace DEM::RPG
{

struct CEquippedComponent
{
	Game::HEntity                  OwnerID;
	std::vector<Data::PRefCounted> Modifiers;
	//!!!instead of modifiers, store modified stats! by ID?
	sol::function                  FnUpdateEquipped;
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<DEM::RPG::CEquippedComponent>() { return "DEM::RPG::CEquippedComponent"; }
template<> constexpr auto RegisterMembers<DEM::RPG::CEquippedComponent>()
{
	return std::make_tuple(); // This component is not persistent.
}

}
