#pragma once
#include <Data/Metadata.h>

// Optional properties and limits for an entity able to contain items

namespace DEM::RPG
{

struct CItemContainerComponent
{
	float MaxWeight = -1.f;
	float MaxVolume = -1.f;
	bool  Temporary = false; // Destroy entity when becomes empty
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<RPG::CItemContainerComponent>() { return "DEM::RPG::CItemContainerComponent"; }
template<> inline constexpr auto RegisterMembers<RPG::CItemContainerComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CItemContainerComponent, 1, MaxWeight),
		DEM_META_MEMBER_FIELD(RPG::CItemContainerComponent, 2, MaxVolume),
		DEM_META_MEMBER_FIELD(RPG::CItemContainerComponent, 3, Temporary)
	);
}

}
