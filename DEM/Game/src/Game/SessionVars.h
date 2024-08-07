#pragma once
#include <Core/RTTIBaseClass.h>
#include <Data/VarStorage.h>

// Global variables belonging to the game session

namespace DEM::Game
{
class CGameSession;
using CSessionVarStorage = CVarStorage<bool, int, float, std::string, CStrID>;

class CSessionVars : public ::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(CSessionVars, ::Core::CRTTIBaseClass);

protected:

	CGameSession& _Session;

public:

	CSessionVarStorage Persistent;
	CSessionVarStorage Runtime;

	CSessionVars(CGameSession& Owner) : _Session(Owner) {}
};

}
