#include "QuestManager.h"
#include <Game/GameSession.h>
#include <Scripting/Flow/ConditionRegistry.h>

namespace DEM::RPG
{

CQuestManager::CQuestManager(Game::CGameSession& Owner)
	: _Session(Owner)
{
}
//---------------------------------------------------------------------

void CQuestManager::LoadState()
{
	// load _FinishedQuests
	// load a list of active quest IDs
	std::vector<CStrID> ActiveQuestIDs;
	for (CStrID QuestID : ActiveQuestIDs)
		HandleQuestStart(QuestID, nullptr, true);

	ProcessQueue(); //???or call manually in external game loading code?
}
//---------------------------------------------------------------------

void CQuestManager::LoadQuests(const Data::PParams& Desc)
{
	DEM::ParamsFormat::DeserializeAppend(Data::CData(Desc), _Quests);
}
//---------------------------------------------------------------------

const CQuestData* CQuestManager::FindQuestData(CStrID ID) const
{
	auto It = _Quests.find(ID);
	return (It != _Quests.cend()) ? &It->second : nullptr;
}
//---------------------------------------------------------------------

bool CQuestManager::HandleQuestStart(CStrID ID, const Flow::CFlowVarStorage* pVars, bool Loading)
{
	// To start the quest again, user must call ResetQuest. It never happens automatically
	// from quest settings and therefore it is safe from infinite loops in a queue.
	if (_FinishedQuests.find(ID) != _FinishedQuests.cend()) return false;

	auto* pQuestData = FindQuestData(ID);
	if (!pQuestData) return false;

	auto [ItActiveQuest, Inserted] = _ActiveQuests.try_emplace(ID);
	if (!Inserted) return false;

	ItActiveQuest->second.pQuestData = pQuestData;

	// Do one time start logic, it should not run on loading already active quests
	if (!Loading)
	{
		// execute OnStart logic, either Lua or Flow
		//???what if Lua script will want to save state and to keep it until the quest end? store lua table? or forbid?
		//???need flow? could be useful, but flow here is not intended to last multiple frames!
		//???control by data type in the desc? treat PParams arg as a flow and string as a lua? what if want a file? CStrID and see extension?
		//???could be a special universal script type with complex deserialization logic?
		//???would want to have one script asset for multiple quest objectives, to reduce number of files? load to some table, internally q1 = {}; function q1:OnStart(...) and find by name?
		//???maybe general purpose script asset should simply listen OnQuestStarted and check ID, instead of personal script? personal is clearer!

		OnQuestStarted(ID);

		// Activate/finish dependent quests before testing outcomes
		for (const auto [DependentID, DepOutcomeID] : pQuestData->EndQuests)
			_ChangeQueue.emplace_back(DependentID, DepOutcomeID, pVars);
		for (CStrID DependentID : pQuestData->StartQuests)
			_ChangeQueue.emplace_back(DependentID, CStrID::Empty, pVars);
	}

	// Quest without outcomes can be only finished by its parent or by explicit SetQuestOutcome call
	for (const auto& [OutcomeID, OutcomeData] : pQuestData->Outcomes)
	{
		// Outcomes without a condition can be selected only by explicit SetQuestOutcome call
		if (!OutcomeData.Condition.Type) continue;

		// Evaluate outcome condition immediately to catch already completed quests
		const auto& Cond = OutcomeData.Condition;
		if (Flow::EvaluateCondition(Cond, _Session, pVars))
		{
			_ChangeQueue.emplace_back(ID, OutcomeID, pVars);
		}
		else if (const auto* pConditions = _Session.FindFeature<Flow::CConditionRegistry>())
		{
			// Not satisfied condition will be re-tested on one of relevant events
			if (auto* pCondition = pConditions->FindCondition(Cond.Type))
			{
				pCondition->SubscribeRelevantEvents(ItActiveQuest->second.Subs, { Cond, _Session, pVars }, [this, ID, OutcomeID, &Cond](const Flow::CFlowVarStorage* pEventVars)
				{
					if (Flow::EvaluateCondition(Cond, _Session, pEventVars))
					{
						_ChangeQueue.emplace_back(ID, OutcomeID, pEventVars);
						ProcessQueue();
					}
				});
			}
		}
	}

	return true;
}
//---------------------------------------------------------------------

bool CQuestManager::HandleQuestCompletion(CStrID ID, CStrID OutcomeID, const Flow::CFlowVarStorage* pVars)
{
	// Finish an active quest even if it is already recorded in _FinishedQuests. This is possible only by mistake.
	auto ItActiveQuest = _ActiveQuests.find(ID);
	if (ItActiveQuest != _ActiveQuests.cend())
	{
		//!!!can copy vars from event trigger and pass here for postprocessing and outcome mod
		// execute outcome script, pass outcome ID and reward for optional modification
		// apply reward

		_ActiveQuests.erase(ItActiveQuest);
	}

	// To change the quest outcome, user must call ResetQuest. It never happens automatically
	// from quest settings and therefore it is safe from infinite loops in a queue.
	auto [_, Inserted] = _FinishedQuests.try_emplace(ID, OutcomeID);
	if (!Inserted) return false;

	// Automatically end child quests with the same outcome ID
	for (auto& [ChildID, ActiveQuest] : _ActiveQuests)
		if (ActiveQuest.pQuestData->ParentID == ID)
			_ChangeQueue.emplace_back(ChildID, OutcomeID, pVars);

	// Activate/finish dependent quests
	if (auto* pQuestData = FindQuestData(ID))
	{
		auto ItOutcome = pQuestData->Outcomes.find(OutcomeID);
		if (ItOutcome != pQuestData->Outcomes.cend())
		{
			for (const auto [DependentID, DepOutcomeID] : ItOutcome->second.EndQuests)
				_ChangeQueue.emplace_back(DependentID, DepOutcomeID, pVars);
			for (CStrID DependentID : ItOutcome->second.StartQuests)
				_ChangeQueue.emplace_back(DependentID, CStrID::Empty, pVars);
		}
	}

	OnQuestCompleted(ID, OutcomeID);

	return true;
}
//---------------------------------------------------------------------

void CQuestManager::StartQuest(CStrID ID, const Flow::CFlowVarStorage* pVars)
{
	_ChangeQueue.emplace_back(ID, CStrID::Empty, pVars);
	ProcessQueue();
}
//---------------------------------------------------------------------

void CQuestManager::SetQuestOutcome(CStrID ID, CStrID OutcomeID, const Flow::CFlowVarStorage* pVars)
{
	_ChangeQueue.emplace_back(ID, OutcomeID, pVars);
	ProcessQueue();
}
//---------------------------------------------------------------------

// Quest system must preserve change order and linearize changes chaotically triggered by quest interdependencies
void CQuestManager::ProcessQueue()
{
	if (_IsInQueueProcessing) return;
	_IsInQueueProcessing = true;

	while (!_ChangeQueue.empty())
	{
		const auto Record = std::move(_ChangeQueue.front());
		_ChangeQueue.pop_front();

		if (Record.OutcomeID)
			HandleQuestCompletion(Record.QuestID, Record.OutcomeID, Record.Vars.get());
		else
			HandleQuestStart(Record.QuestID, Record.Vars.get(), false);
	}

	_IsInQueueProcessing = false;
}
//---------------------------------------------------------------------

}
