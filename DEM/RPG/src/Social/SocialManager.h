#pragma once
#include <Core/RTTIBaseClass.h>

// Social dynamics manager keeps track of factions, reputation, disposition, crimes etc

namespace DEM::Game
{
	class CGameSession;
	class CGameWorld;
}

namespace DEM::RPG
{

class CSocialManager: public ::Core::CRTTIBaseClass
{
private:

	Game::CGameSession& _Session;

	// faction map ID -> info
	//  - map of faction ID -> float coeff (if doesn't contain self, add default 1.f)
	//  - deed to reputation coeff
	//  - float good reputation
	//  - float bad reputation
	//  - script and/or crime rules
	//  - ???unpaid crime accumulator?
	// party personality traits map CStrID -> float (or better use int here?)
	// balance constants (here or load everything to global/session vars?)

public:

	CSocialManager(Game::CGameSession& Owner);
};

}
