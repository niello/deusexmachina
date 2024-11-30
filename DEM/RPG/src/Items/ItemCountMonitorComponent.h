#pragma once
#include <Data/Metadata.h>
#include <StdDEM.h>

// Tracks previous count of the item. This enables systems to check if the count has changed.
// Adding this component is much easier than trying to monitor changes in count of all item
// stacks in all places where it could happen.
// This component is not persistent.

namespace DEM::RPG
{

struct CItemCountMonitorComponent
{
	U32 PrevCount = 0;
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<DEM::RPG::CItemCountMonitorComponent>() { return "DEM::RPG::CItemCountMonitorComponent"; }
template<> constexpr auto RegisterMembers<DEM::RPG::CItemCountMonitorComponent>()
{
	return std::make_tuple(); // This component is not persistent.
}

}
