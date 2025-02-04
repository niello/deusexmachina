#pragma once
#include <Core/RTTIBaseClass.h>
#include <Data/StringID.h>
#include <Data/Ptr.h>
#include <Data/Metadata.h>
#include <map>

// Social dynamics manager keeps track of factions, reputation, disposition, crimes etc

namespace DEM::Game
{
	class CGameSession;
	class CGameWorld;
}

namespace Data
{
	using PParams = Ptr<class CParams>;
}

namespace DEM::RPG
{

struct CFactionInfo
{
	std::map<CStrID, float> Relations;
	float GoodReputation = 0.f; //???or int, and divide by deed to reputation coeff when calc result?
	float BadReputation = 0.f; //???or int, and divide by deed to reputation coeff when calc result?
	//  - deed to reputation coeff
	//  - script and/or crime rules
	//  - ???unpaid crime accumulator?
};

class CSocialManager: public ::Core::CRTTIBaseClass
{
private:

	Game::CGameSession& _Session;

	std::map<CStrID, CFactionInfo> _Factions;
	std::map<CStrID, U32>          _PartyTraits;   // Counters for various types of behaviour expressed by party members (Kind, Aggressive, Honest etc)
	std::set<CStrID>               _PartyFactions; //???need here? 'Party' by default but can add more by joining factions or using disguise.
	// balance constants (here or load everything to global/session vars?)

public:

	CSocialManager(Game::CGameSession& Owner);

	void                LoadFactions(const Data::PParams& Desc);
	const CFactionInfo* FindFaction(CStrID ID) const;
	void                AddPartyFaction(CStrID ID) { _PartyFactions.insert(ID); }
	U32                 GetPartyTrait(CStrID ID) const;
	const auto&         GetPartyFactions() const { return _PartyFactions; } //???or need a single Party faction and assign additional factions to characters?
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<RPG::CFactionInfo>() { return "DEM::RPG::CFactionInfo"; }
template<> constexpr auto RegisterMembers<RPG::CFactionInfo>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CFactionInfo, Relations),
		DEM_META_MEMBER_FIELD(RPG::CFactionInfo, GoodReputation),
		DEM_META_MEMBER_FIELD(RPG::CFactionInfo, BadReputation)
	);
}
static_assert(CMetadata<RPG::CFactionInfo>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

}
