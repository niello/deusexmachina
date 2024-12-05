#pragma once
#include <Core/RTTIBaseClass.h>
#include <Quests/QuestData.h>
#include <Events/Signal.h>
#include <Data/Ptr.h>
#include <deque>
#include <memory>

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

	// TODO: could use shared_ptr to avoid multiple copies of the same param set! But need to enforce this in args.
	//!!!lua can construct shared ptr to via special factory binding!
	using PFlowVarStorage = std::unique_ptr<Flow::CFlowVarStorage>;

	struct CActiveQuest
	{
		const CQuestData*                pQuestData = nullptr;
		std::vector<Events::CConnection> Subs;
		// variable storage
		// if needed, flow player and cached lua objects/functions
	};

	struct CQueueRecord
	{
		CStrID          QuestID;
		CStrID          OutcomeID; // Empty for quest activation
		PFlowVarStorage Vars;      // Optional variables, e.g. from a triggering event

		CQueueRecord(CStrID QuestID_, CStrID OutcomeID_, const Flow::CFlowVarStorage* pVars)
			: QuestID(QuestID_)
			, OutcomeID(OutcomeID_)
			, Vars(pVars ? std::make_unique<Flow::CFlowVarStorage>(*pVars) : nullptr)
		{
		}
	};

	Game::CGameSession&                      _Session;

	std::unordered_map<CStrID, CQuestData>   _Quests;
	std::unordered_map<CStrID, CActiveQuest> _ActiveQuests;
	std::unordered_map<CStrID, CStrID>       _FinishedQuests;
	std::deque<CQueueRecord>                 _ChangeQueue; //  A queue of pending operations Quest ID -> Outcome ID or activation if empty

	bool                                     _IsInQueueProcessing = false;

	bool HandleQuestStart(CStrID ID, const Flow::CFlowVarStorage* pVars, bool Loading);
	bool HandleQuestCompletion(CStrID ID, CStrID OutcomeID, const Flow::CFlowVarStorage* pVars);

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

	void              LoadState();

	void              LoadQuests(const Data::PParams& Desc);
	const CQuestData* FindQuestData(CStrID ID) const;
	void              StartQuest(CStrID ID, const Flow::CFlowVarStorage* pVars = nullptr);
	void              SetQuestOutcome(CStrID ID, CStrID OutcomeID, const Flow::CFlowVarStorage* pVars = nullptr);
	//GetQuestState - inactive, active, completed (with outcome). Return pair? Check queue!!!
	//ResetQuest - returns to not activated state from either active or finished state, remove from queue
	//ResetAllQuests

	void              ProcessQueue();
};

}
