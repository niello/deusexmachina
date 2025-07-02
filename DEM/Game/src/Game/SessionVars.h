#pragma once
#include <Core/RTTIBaseClass.h>
#include <Game/GameVarStorage.h>

// Global variables belonging to the game session

namespace DEM::Game
{
class CGameSession;

class CSessionVars : public DEM::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(CSessionVars, DEM::Core::CRTTIBaseClass);

protected:

	CGameSession& _Session;

public:

	CGameVarStorage Persistent;
	CGameVarStorage Runtime;

	CSessionVars(CGameSession& Owner) : _Session(Owner) {}
};

}
