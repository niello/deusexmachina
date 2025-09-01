#pragma once
#include <Data/Metadata.h>
#include <map>

// Social stats for NPC

namespace DEM::RPG
{

struct CSocialComponent
{
	float                       Disposition = 0.f;
	std::set<CStrID>            Factions;
	std::map<CStrID, float>     FactionDispositonCoeffs;
	std::map<CStrID, float>     TraitDispositonCoeffs;
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<RPG::CSocialComponent>() { return "DEM::RPG::CSocialComponent"; }
template<> constexpr auto RegisterMembers<RPG::CSocialComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CSocialComponent, Disposition),
		DEM_META_MEMBER_FIELD(RPG::CSocialComponent, Factions),
		DEM_META_MEMBER_FIELD(RPG::CSocialComponent, FactionDispositonCoeffs),
		DEM_META_MEMBER_FIELD(RPG::CSocialComponent, TraitDispositonCoeffs)
	);
}

}
