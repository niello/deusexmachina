#pragma once
#include <Data/Metadata.h>
#include <StdDEM.h>

// Common item properties

namespace DEM::RPG
{

struct CItemComponent
{
	CStrID      WorldModelID;
	CStrID      WorldPhysicsID;
	CStrID      UIIcon;
	std::string UIName;
	float       Weight = 0.f;
	float       Volume = 0.f;
	U32         Price = 0;
	bool        AlwaysShowCount = false;
};

struct CCurrencyComponent {}; // A simple flag component for currencies (items with an always stable price)

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<RPG::CItemComponent>() { return "DEM::RPG::CItemComponent"; }
template<> constexpr auto RegisterMembers<RPG::CItemComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CItemComponent, WorldModelID),
		DEM_META_MEMBER_FIELD(RPG::CItemComponent, WorldPhysicsID),
		DEM_META_MEMBER_FIELD(RPG::CItemComponent, UIIcon),
		DEM_META_MEMBER_FIELD(RPG::CItemComponent, UIName),
		DEM_META_MEMBER_FIELD(RPG::CItemComponent, Weight),
		DEM_META_MEMBER_FIELD(RPG::CItemComponent, Volume),
		DEM_META_MEMBER_FIELD(RPG::CItemComponent, Price),
		DEM_META_MEMBER_FIELD(RPG::CItemComponent, AlwaysShowCount)
	);
}
static_assert(CMetadata<RPG::CItemComponent>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

}
