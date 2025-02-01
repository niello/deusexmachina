#pragma once
#include <Data/Metadata.h>
#include <Data/StringID.h>
#include <StdDEM.h>

// Allows an item to serve as a lockpick

namespace DEM::RPG
{

struct CCurrencyComponent
{
	std::set<CStrID> Factions; // If empty, the currency is accepted as such by all factions
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<RPG::CCurrencyComponent>() { return "DEM::RPG::CCurrencyComponent"; }
template<> constexpr auto RegisterMembers<RPG::CCurrencyComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CCurrencyComponent, Factions)
	);
}
static_assert(CMetadata<RPG::CCurrencyComponent>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

}
