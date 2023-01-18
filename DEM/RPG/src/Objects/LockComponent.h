#pragma once
#include <Data/Metadata.h>
#include <StdDEM.h>

// Adds a lock to the entity. Lock blocks some actions, like opening the door.
// Lock can be opened by key or lockpicked.

namespace DEM::RPG //???Shantara2?
{

struct CLockComponent
{
	//!!!TODO: serialize U8 to CData!
	int Difficulty = 0; // Difficulty of lockpicking this lock
	int Jamming = 0;    // Difficulty of repairing this jammed lock (zero unless jammed)
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::RPG::CLockComponent>() { return "DEM::RPG::CLockComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CLockComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CLockComponent, 1, Difficulty),
		DEM_META_MEMBER_FIELD(RPG::CLockComponent, 2, Jamming)
	);
}

}
