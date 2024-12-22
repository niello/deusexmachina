#pragma once
#include <Core/RTTIBaseClass.h>
#include <Data/VarStorage.h>

// Global variables belonging to the game session

namespace DEM::Game
{
class CGameSession;

class CSessionVars : public ::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(CSessionVars, ::Core::CRTTIBaseClass);

protected:

	CGameSession& _Session;

public:

	CBasicVarStorage Persistent;
	CBasicVarStorage Runtime;

	CSessionVars(CGameSession& Owner) : _Session(Owner) {}
};

}
