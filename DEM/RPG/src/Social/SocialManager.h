#pragma once
#include <Core/RTTIBaseClass.h>
#include <Data/StringID.h>
#include <map>

// Social dynamics manager keeps track of factions, reputation, disposition, crimes etc

namespace DEM::Game
{
	class CGameSession;
	class CGameWorld;
}

namespace DEM::RPG
{

struct CFactionInfo
{
	std::map<CStrID, float> FactionDispositonCoeffs;
	//  - map of faction ID -> float coeff (if doesn't contain self, add default 1.f)
	//  - deed to reputation coeff
	float GoodReputation = 0.f; //???or int, and divide by deed to reputation coeff when calc result?
	float BadReputation = 0.f; //???or int, and divide by deed to reputation coeff when calc result?
	//  - script and/or crime rules
	//  - ???unpaid crime accumulator?
};

class CSocialManager: public ::Core::CRTTIBaseClass
{
private:

	Game::CGameSession& _Session;

	std::map<CStrID, CFactionInfo> _Factions;
	std::map<CStrID, U32>          _PartyTraits;
	std::set<CStrID>               _PartyFactions; //???need here? 'Party' by default but can add more by joining factions or using disguise.
	// balance constants (here or load everything to global/session vars?)

public:

	CSocialManager(Game::CGameSession& Owner);

	const CFactionInfo* FindFaction(CStrID ID) const;
	U32                 GetPartyTrait(CStrID ID) const;
	const auto&         GetPartyFactions() const { return _PartyFactions; }
};

}
