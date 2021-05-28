#pragma once
#include <Data/Metadata.h>
#include <StdDEM.h>

// Stores an inventory of a character for a Shantara2 role system
//TODO: RPG common if it is simply a list of item entities?

namespace DEM::Sh2
{

struct CInventoryComponent
{
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::Sh2::CInventoryComponent>() { return "DEM::Sh2::CInventoryComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::Sh2::CInventoryComponent>()
{
	return std::make_tuple
	(
	);
}

}
