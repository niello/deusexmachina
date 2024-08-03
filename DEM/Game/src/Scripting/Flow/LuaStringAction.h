#pragma once
#include <Scripting/Flow/FlowPlayer.h>

// An action that executes a Lua script string

namespace DEM::Flow
{

class CLuaStringAction : public Flow::IFlowAction
{
	FACTORY_CLASS_DECL;

public:

	virtual void OnStart(Game::CGameSession& Session) override {}
	virtual void Update(Flow::CUpdateContext& Ctx) override;
};

}
