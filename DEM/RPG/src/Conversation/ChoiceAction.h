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

	//!!!???TODO: allow choices to be from different speakers?! don't store speaker here, get it from phrase action when it is chosen and played!
	Game::HEntity                       _Speaker;
	std::vector<std::string>            _ChoiceTexts;
	std::vector<const Flow::CFlowLink*> _ChoiceLinks;
	Events::CConnection                 _ChoiceMadeConn;
	size_t                              _Choice;

	static void CollectChoicesFromLink(CChoiceAction& Root, const Flow::CFlowLink& Link, Game::CGameSession& Session);
	static void CollectChoices(CChoiceAction& Root, const Flow::CFlowActionData& Curr, Game::CGameSession& Session);

public:

	virtual void OnStart(Game::CGameSession& Session) override;
	virtual void Update(Flow::CUpdateContext& Ctx) override;
};

}
