#pragma once
#include <Data/Metadata.h>
#include <StdDEM.h>

// Adds a lock to the entity. Lock blocks some actions, like opening the door.
// Lock can be opened by key or lockpicked.

namespace DEM::RPG //???Shantara2?
{

struct CLockComponent
{
	int Difficulty; //!!!TODO: serialize U8 to CData!
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::RPG::CLockComponent>() { return "DEM::RPG::CLockComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CLockComponent>()
{
	return std::make_tuple
	(
		Member(1, "Difficulty", &DEM::RPG::CLockComponent::Difficulty, &DEM::RPG::CLockComponent::Difficulty)
	);
}

}
