#pragma once
#include <Core/RTTIBaseClass.h>
#include <Quests/QuestData.h>
#include <Events/Signal.h>
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

	struct CActiveQuest
	{
		const CQuestData*                pQuestData = nullptr;
		std::vector<Events::CConnection> Subs; //???can use the same CConnection for event dispatcher subscriptions?
		// variable storage
		// if needed, flow player and cached lua objects/functions
	};

	Game::CGameSession&                      _Session;

	std::unordered_map<CStrID, CQuestData>   _Quests;
	std::unordered_map<CStrID, CActiveQuest> _ActiveQuests;

public:

	Events::CSignal<void(CStrID)>         OnQuestStarted;   // QuestID
	Events::CSignal<void(CStrID)>         OnQuestReset;     // QuestID
	Events::CSignal<void(CStrID, CStrID)> OnQuestCompleted; // QuestID, OutcomeID

	//???!!!on subscription triggered, can pass data from the event to outcome calculation? or always calculated from the world state?
	//!!!e.g. destroy with fire may not work without events! fire damage type is recorded only in a death event! can combine with flow or need Lua?
	//???set all or chosen (by ID) params to flow vars context?! OnDead(EventParams) -> Flow.SetVar("DamageType", EventParams.DamageType), RecalcOutcomeConditions()
	//???instead of flow script, use only declarative conditions and handle them in C++ with additional flexibility through Lua?
	//or event-listening flow action will catch an event and evaluate conditions in a context of this event? but how, if there is no such param in EvaluateCondition?

	CQuestManager(Game::CGameSession& Owner);

	void              LoadQuests(const Data::PParams& Desc);
	const CQuestData* FindQuestData(CStrID ID) const;
	bool              StartQuest(CStrID ID);
	//GetQuestState - inactive, active, completed (with outcome). Return pair?
	//SetQuestOutcome
	//ActivateQuest - does nothiong if active, what if finished with outcome?
	//ResetQuest - returns to not activated state
};

}
