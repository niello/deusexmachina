#pragma once
#include <Data/Metadata.h>

// Character AI state

namespace DEM::AI
{

struct CPassiveSensor
{
	// modality
	// radius
	// center pos or node

	// collision shape
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<DEM::AI::CPassiveSensor>() { return "DEM::AI::CPassiveSensor"; }
template<> constexpr auto RegisterMembers<DEM::AI::CPassiveSensor>()
{
	return std::make_tuple
	(
	);
}

}
