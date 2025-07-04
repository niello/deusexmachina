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

	enum class EState : U8
	{
		Created,
		Started,
		Finished,
		Error
	};

	Game::HEntity       _Speaker;
	Events::CConnection _PhraseEndConn;
	EState              _State = EState::Created;

public:

	static bool IsMatchingConversation(Game::HEntity Speaker, Game::CGameSession& Session, const Game::CGameVarStorage& Vars);

	virtual void OnStart(Game::CGameSession& Session) override;
	virtual void Update(Flow::CUpdateContext& Ctx) override;
};

}
