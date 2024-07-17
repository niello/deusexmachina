#pragma once
#include <Scripting/Flow/FlowPlayer.h>
#include <Game/ECS/Entity.h>

// An action for speaking a phrase either in a foreground UI or in a background

namespace DEM::RPG
{

class CPhraseAction : public Flow::IFlowAction
{
	FACTORY_CLASS_DECL;

protected:

	Game::HEntity _Speaker;
	float         _dt = 0.f;

public:

	virtual void OnStart() override;
	virtual void Update(Flow::CUpdateContext& Ctx) override;
};

}
