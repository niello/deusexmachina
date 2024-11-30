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
	int    Difficulty = 0; // Difficulty of lockpicking this lock
	int    Jamming = 0;    // Difficulty of repairing this jammed lock (zero unless jammed)

	CStrID KeyItemID;
	U32    KeyItemCount = 1;
	bool   KeyConsume = true;
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<RPG::CLockComponent>() { return "DEM::RPG::CLockComponent"; }
template<> constexpr auto RegisterMembers<RPG::CLockComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CLockComponent, Difficulty),
		DEM_META_MEMBER_FIELD(RPG::CLockComponent, Jamming),
		DEM_META_MEMBER_FIELD(RPG::CLockComponent, KeyItemID),
		DEM_META_MEMBER_FIELD(RPG::CLockComponent, KeyItemCount),
		DEM_META_MEMBER_FIELD(RPG::CLockComponent, KeyConsume)
	);
}
static_assert(CMetadata<RPG::CLockComponent>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

}
