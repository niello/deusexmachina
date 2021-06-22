#pragma once
#include <Data/Metadata.h>
#include <StdDEM.h>

// Common item properties

namespace DEM::RPG
{

struct CItemComponent
{
	CStrID      InLocationModelID;
	CStrID      InLocationPhysicsID;
	CStrID      UIIcon;
	std::string UIName;
	float       Weight = 0.f;
	float       Volume = 0.f;
	U32         Price = 0;
	bool        AlwaysShowCount = false;
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
		DEM_META_MEMBER_FIELD(RPG::CItemComponent, 2, InLocationPhysicsID),
		DEM_META_MEMBER_FIELD(RPG::CItemComponent, 3, UIIcon),
		DEM_META_MEMBER_FIELD(RPG::CItemComponent, 4, UIName),
		DEM_META_MEMBER_FIELD(RPG::CItemComponent, 5, Weight),
		DEM_META_MEMBER_FIELD(RPG::CItemComponent, 6, Volume),
		DEM_META_MEMBER_FIELD(RPG::CItemComponent, 7, Price),
		DEM_META_MEMBER_FIELD(RPG::CItemComponent, 8, AlwaysShowCount)
	);
}

}
