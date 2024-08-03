#pragma once
#include <Scripting/Flow/FlowPlayer.h>
#include <Game/ECS/Entity.h>

// An action for providing answer choices for a player in a foreground UI

namespace DEM::RPG
{

class CChoiceAction : public Flow::IFlowAction
{
	FACTORY_CLASS_DECL;

protected:

	enum class EState : U8
	{
		Created,
		Started,
		Finished
	};

	Game::HEntity       _Speaker;
	Events::CConnection _ChoiceMadeConn;
	EState              _State = EState::Created;

public:

	virtual void OnStart() override;
	virtual void Update(Flow::CUpdateContext& Ctx) override;
};

}
