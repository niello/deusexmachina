#pragma once
#include <Core/RTTIBaseClass.h>
#include <Quests/QuestData.h>
#include <Events/Signal.h>
#include <Data/Ptr.h>
#include <Scripting/SolLow.h>
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

enum class EQuestState : U8
{
	NotStarted,
	Active,
	Completed
};

class CQuestManager : public ::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(CQuestManager, ::Core::CRTTIBaseClass);

public:

	using PFlowVarStorage = std::shared_ptr<Flow::CFlowVarStorage>;

private:

	struct CActiveQuest
	{
		const CQuestData*                pQuestData = nullptr;
		PFlowVarStorage                  Vars; //???need? save persistent?
		sol::function                    FnOnStart;
		sol::function                    FnOnComplete;
		std::vector<Events::CConnection> Subs;
		//???store activation time in the world time? save persistent? or should it be stored in Vars?
	};

	struct CQueueRecord
	{
		CStrID          QuestID;
		CStrID          OutcomeID; // Empty for quest activation
		PFlowVarStorage Vars;      // Optional variables, e.g. from a triggering event

		CQueueRecord(CStrID QuestID_, CStrID OutcomeID_, PFlowVarStorage Vars_)
			: QuestID(QuestID_)
			, OutcomeID(OutcomeID_)
			, Vars(std::move(Vars_))
		{
		}
	};

	Game::CGameSession&                      _Session;

	std::unordered_map<CStrID, CQuestData>   _Quests;
	std::unordered_map<CStrID, CActiveQuest> _ActiveQuests;
	std::unordered_map<CStrID, CStrID>       _FinishedQuests;
	std::deque<CQueueRecord>                 _ChangeQueue; //  A queue of pending operations Quest ID -> Outcome ID or activation if empty

	bool                                     _IsInQueueProcessing = false;

	bool HandleQuestStart(CStrID ID, PFlowVarStorage Vars, bool Loading);
	bool HandleQuestCompletion(CStrID ID, CStrID OutcomeID, PFlowVarStorage Vars);
	void EnqueueQuestStart(CStrID ID, PFlowVarStorage Vars);
	void EnqueueQuestCompletion(CStrID ID, CStrID OutcomeID, PFlowVarStorage Vars);

public:

	Events::CSignal<void(CStrID)>         OnQuestStarted;   // QuestID
	Events::CSignal<void(CStrID)>         OnQuestReset;     // QuestID
	Events::CSignal<void(CStrID, CStrID)> OnQuestCompleted; // QuestID, OutcomeID

	CQuestManager(Game::CGameSession& Owner);

	void                           LoadState();

	void                           LoadQuests(const Data::PParams& Desc);
	const CQuestData*              FindQuestData(CStrID ID) const;
	void                           StartQuest(CStrID ID, PFlowVarStorage Vars = {});
	void                           SetQuestOutcome(CStrID ID, CStrID OutcomeID, PFlowVarStorage Vars = {});
	void                           ResetQuest(CStrID ID);
	std::pair<EQuestState, CStrID> GetQuestState(CStrID ID) const;
	CStrID                         GetQuestOutcome(CStrID ID) const;
	bool                           IsQuestActive(CStrID ID) const { return _ActiveQuests.find(ID) != _ActiveQuests.cend(); }

	void                           ProcessQueue();
};

}
