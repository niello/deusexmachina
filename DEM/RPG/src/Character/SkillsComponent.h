#pragma once
#include <Data/Metadata.h>
#include <StdDEM.h>

// A set of character skills for Shantara2 role system

namespace DEM::Sh2
{

struct CSkillsComponent
{
	//!!!TODO: serialize U8 to CData!
	int Lockpicking = 0;
	int Mechanics = 0;
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::Sh2::CSkillsComponent>() { return "DEM::Sh2::CSkillsComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::Sh2::CSkillsComponent>()
{
	return std::make_tuple
	(
		Member(1, "Lockpicking", &DEM::Sh2::CSkillsComponent::Lockpicking, &DEM::Sh2::CSkillsComponent::Lockpicking),
		Member(2, "Mechanics", &DEM::Sh2::CSkillsComponent::Mechanics, &DEM::Sh2::CSkillsComponent::Mechanics)
	);
}

}
