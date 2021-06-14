#pragma once
#include <Data/Metadata.h>
#include <StdDEM.h>

// Common item properties

namespace DEM::RPG
{

struct CItemComponent
{
	CStrID InLocationModelID;
	float  Weight = 0.f;
	float  Volume = 0.f;
	U32    Price = 0;
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<RPG::CItemComponent>() { return "DEM::RPG::CItemComponent"; }
template<> inline constexpr auto RegisterMembers<RPG::CItemComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CItemComponent, 1, InLocationModelID),
		DEM_META_MEMBER_FIELD(RPG::CItemComponent, 2, Weight),
		DEM_META_MEMBER_FIELD(RPG::CItemComponent, 3, Volume),
		DEM_META_MEMBER_FIELD(RPG::CItemComponent, 4, Price)
	);
}

}
