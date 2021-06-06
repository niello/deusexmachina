#pragma once
#include <Data/Metadata.h>
#include <StdDEM.h>

// Common item properties

namespace DEM::RPG
{

struct CItemComponent
{
	float Weight = 0.f;
	float Volume = 0.f;
	U32   Price = 0;
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<RPG::CItemComponent>() { return "DEM::RPG::CItemComponent"; }
template<> inline constexpr auto RegisterMembers<RPG::CItemComponent>()
{
	return std::make_tuple
	(
		Member(1, "Weight", &RPG::CItemComponent::Weight, &RPG::CItemComponent::Weight),
		Member(2, "Volume", &RPG::CItemComponent::Volume, &RPG::CItemComponent::Volume)//,
		//Member(3, "Price", &RPG::CItemComponent::Price, &RPG::CItemComponent::Price)
	);
}

}
