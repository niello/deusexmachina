#pragma once
#include <Data/Metadata.h>
#include <Game/ECS/Entity.h>
#include <sol/sol.hpp>

// A component that marks individual item stacks as equipped and holds their modifiers applied to the equipment owner.
// The only reason to have modifiers here is to hold strong references. Processing is done in CModifiableParameter.
// This component is not persistent.

namespace DEM::RPG
{

struct CEquippedComponent
{
	Game::HEntity                  OwnerID;
	std::vector<Data::PRefCounted> Modifiers;
	sol::function                  FnUpdateEquipped;
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::RPG::CEquippedComponent>() { return "DEM::RPG::CEquippedComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CEquippedComponent>()
{
	return std::make_tuple(); // This component is not persistent.
}

}
