#pragma once
#include <Scripting/Flow/FlowPlayer.h>
#include <Game/ECS/Entity.h>

// An action for engaging an actor in a conversation

namespace DEM::RPG
{

class CEngageInConversationAction : public Flow::IFlowAction
{
	FACTORY_CLASS_DECL;

protected:

	Game::HEntity _Actor;
	bool          _DisengageFromCurrent = false;
	bool          _Optional = true;

public:

	virtual void OnStart(Game::CGameSession& Session) override;
	virtual void Update(Flow::CUpdateContext& Ctx) override;
};

}
