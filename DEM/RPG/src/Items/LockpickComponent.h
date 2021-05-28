#pragma once
#include <Data/Metadata.h>
#include <StdDEM.h>

// Allows an item to serve as a lockpick

namespace DEM::Sh2
{

struct CLockpickComponent
{
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::Sh2::CLockpickComponent>() { return "DEM::Sh2::CLockpickComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::Sh2::CLockpickComponent>()
{
	return std::make_tuple
	(
	);
}

}
