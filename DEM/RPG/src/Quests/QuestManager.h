#pragma once
#include <Core/RTTIBaseClass.h>
#include <Data/Ptr.h>

// Quest system manages current player (character) tasks and their flow (completion, failure,
// opening new tasks etc)

namespace Data
{
	using PParams = Ptr<class CParams>;
}

namespace DEM::Game
{
	class CGameSession;
}

namespace DEM::RPG
{

class CQuestManager : public ::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(CQuestManager, ::Core::CRTTIBaseClass);

private:

	Game::CGameSession& _Session;

public:

	// Quest = quest data ref (1<->1) + flow player, script function cache, subscriptions

	CQuestManager(Game::CGameSession& Owner);

	void LoadQuests(const Data::PParams& Desc);
	//GetQuestState
	//SetQuestOutcome
	//ActivateQuest - does nothiong if active, what if finished with outcome?
	//ResetQuest - returns to not activated state
};

}
