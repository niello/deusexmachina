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

	std::vector<std::string>            _ChoiceTexts;
	std::vector<const Flow::CFlowLink*> _ChoiceLinks;
	std::vector<bool>                   _ChoiceValidFlags; // For debug, isn't filled when not in a conversation debug mode
	Events::CConnection                 _ChoiceMadeConn;
	std::optional<size_t>               _Choice;

	static void CollectChoicesFromLink(CChoiceAction& Root, const Flow::CFlowLink& Link, Game::CGameSession& Session, bool DebugMode, bool IsValid);
	static void CollectChoices(CChoiceAction& Root, const Flow::CFlowActionData& Curr, Game::CGameSession& Session, bool DebugMode, bool IsValid);

public:

	virtual void OnStart(Game::CGameSession& Session) override;
	virtual void Update(Flow::CUpdateContext& Ctx) override;
};

}
