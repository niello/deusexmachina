#pragma once
#include <Scripting/Flow/FlowPlayer.h>
#include <Game/ECS/Entity.h>

// An action for engaging an actor in a conversation

namespace DEM::RPG
{

class CBarterAction : public Flow::IFlowAction
{
	FACTORY_CLASS_DECL;

protected:

	Game::HEntity _Actor;

public:

	virtual void OnStart(Game::CGameSession& Session) override;
	virtual void Update(Flow::CUpdateContext& Ctx) override;
};

}
