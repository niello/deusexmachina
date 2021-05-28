#pragma once
#include <Data/Metadata.h>
#include <StdDEM.h>

// A set of creature stats for Shantara2 role system

namespace DEM::Sh2
{

struct CStatsComponent
{
	//!!!TODO: serialize U8 to CData!
	int Dexterity = 0;
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::Sh2::CStatsComponent>() { return "DEM::Sh2::CStatsComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::Sh2::CStatsComponent>()
{
	return std::make_tuple
	(
		Member(1, "Dexterity", &DEM::Sh2::CStatsComponent::Dexterity, &DEM::Sh2::CStatsComponent::Dexterity)
	);
}

}
