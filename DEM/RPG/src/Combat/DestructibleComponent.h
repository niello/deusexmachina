#pragma once
#include <Data/Metadata.h>
#include <StdDEM.h>

// An entity with a destructible component will be logically destroyed (killed) when it runs out of HP

namespace DEM::RPG
{

struct CDestructibleComponent
{
	int HP = 0;

	// TODO: resistances and immunities. here or in a separate component with hit zones?
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::RPG::CDestructibleComponent>() { return "DEM::RPG::CDestructibleComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CDestructibleComponent>()
{
	return std::make_tuple
	(
		Member(1, "HP", &DEM::RPG::CDestructibleComponent::HP, &DEM::RPG::CDestructibleComponent::HP)
	);
}

}
