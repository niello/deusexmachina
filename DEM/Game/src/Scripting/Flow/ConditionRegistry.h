#pragma once
#include <Core/RTTIBaseClass.h>

// A registry of declarative condition implementations for picking them by type ID

namespace DEM::Game
{
	class CGameSession;
}

namespace DEM::Flow
{

class CConditionRegistry : public ::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(CConditionRegistry, ::Core::CRTTIBaseClass);

protected:

	Game::CGameSession& _Session;

public:

	CConditionRegistry(Game::CGameSession& Owner) : _Session(Owner) {}
};

}
