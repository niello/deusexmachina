#pragma once
#include <Data/Metadata.h>
#include <Data/StringID.h>
#include <Character/ModifiableParameter.h> // FIXME: not character but stats?
#include <map>

// Social stats for NPC

namespace DEM::RPG
{

struct CSocialComponent
{
	CModifiableParameter<float> Disposition = 0.f;
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
		//DEM_META_MEMBER_FIELD(RPG::CSocialComponent, Disposition), // TODO: (de)serialize CModifiableParameter as base value! sol binding via unique traits like ptr?
		DEM_META_MEMBER_FIELD(RPG::CSocialComponent, Factions),
		DEM_META_MEMBER_FIELD(RPG::CSocialComponent, FactionDispositonCoeffs),
		DEM_META_MEMBER_FIELD(RPG::CSocialComponent, TraitDispositonCoeffs)
	);
}

}
