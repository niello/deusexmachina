#pragma once
#include <Data/Metadata.h>
#include <StdDEM.h>

// A set of creature stats for Shantara2 role system

namespace DEM::Sh2
{

enum ECapability : U8 //!!!TODO: serialize U8 to CData!
{
	Move = 0x01,
	Interact = 0x02
};

struct CStatsComponent
{
	//!!!TODO: serialize U8 to CData!
	int Dexterity = 0;
	U8  Capabilities = ECapability::Move | ECapability::Interact; //???!!!serialize enum as String1 | String2 | ...!?
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
