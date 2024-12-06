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

bool CQuestManager::HandleQuestStart(CStrID ID, PFlowVarStorage Vars, bool Loading)
{
	// To start the quest again, user must call ResetQuest. It never happens automatically
	// from quest settings and therefore it is safe from infinite loops in a queue.
	if (_FinishedQuests.find(ID) != _FinishedQuests.cend()) return false;

	auto* pQuestData = FindQuestData(ID);
	if (!pQuestData) return false;

	auto [ItActiveQuest, Inserted] = _ActiveQuests.try_emplace(ID);
	if (!Inserted) return false;

	ItActiveQuest->second.pQuestData = pQuestData;
	ItActiveQuest->second.Vars = std::move(Vars);

	const PFlowVarStorage& QuestVars = ItActiveQuest->second.Vars;

	// Do one time start logic, it should not run on loading already active quests
	if (!Loading)
	{
		// execute OnStart logic, either Lua or Flow
		//???if Vars are mutable here, need to create copy in a script? because our vars may be stored in other quests too!
		//???what if Lua script will want to save state and to keep it until the quest end? store lua table? or forbid?
		//???need flow? could be useful, but flow here is not intended to last multiple frames!
		//???control by data type in the desc? treat PParams arg as a flow and string as a lua? what if want a file? CStrID and see extension?
		//???could be a special universal script type with complex deserialization logic?
		//???would want to have one script asset for multiple quest objectives, to reduce number of files? load to some table, internally q1 = {}; function q1:OnStart(...) and find by name?
		//???maybe general purpose script asset should simply listen OnQuestStarted and check ID, instead of personal script? personal is clearer!

		OnQuestStarted(ID);

		// Activate/finish dependent quests before testing outcomes
		for (const auto [DependentID, DepOutcomeID] : pQuestData->EndQuests)
			_ChangeQueue.emplace_back(DependentID, DepOutcomeID, QuestVars);
		for (CStrID DependentID : pQuestData->StartQuests)
			_ChangeQueue.emplace_back(DependentID, CStrID::Empty, QuestVars);
	}

	// Quest without outcomes can be only finished by its parent or by explicit SetQuestOutcome call
	for (const auto& [OutcomeID, OutcomeData] : pQuestData->Outcomes)
	{
		// Outcomes without a condition can be selected only by explicit SetQuestOutcome call
		if (!OutcomeData.Condition.Type) continue;

		// Evaluate outcome condition immediately to catch already completed quests
		const auto& Cond = OutcomeData.Condition;
		if (Flow::EvaluateCondition(Cond, _Session, QuestVars.get()))
		{
			_ChangeQueue.emplace_back(ID, OutcomeID, QuestVars);
		}
		else if (const auto* pConditions = _Session.FindFeature<Flow::CConditionRegistry>())
		{
			// Not satisfied condition will be re-tested on one of relevant events
			if (auto* pCondition = pConditions->FindCondition(Cond.Type))
			{
				pCondition->SubscribeRelevantEvents(ItActiveQuest->second.Subs, { Cond, _Session, QuestVars.get() }, [this, ID, OutcomeID, &Cond](PFlowVarStorage EventVars)
				{
					if (Flow::EvaluateCondition(Cond, _Session, EventVars.get()))
					{
						_ChangeQueue.emplace_back(ID, OutcomeID, std::move(EventVars));
						ProcessQueue();
					}
				});
			}
		}
	}

	return true;
}
//---------------------------------------------------------------------

bool CQuestManager::HandleQuestCompletion(CStrID ID, CStrID OutcomeID, PFlowVarStorage Vars)
{
	n_assert(OutcomeID);
	if (!OutcomeID) return false;

	const CQuestData* pQuestData = nullptr;

	// Finish an active quest even if it is already recorded in _FinishedQuests. This is possible only by mistake.
	auto ItActiveQuest = _ActiveQuests.find(ID);
	if (ItActiveQuest != _ActiveQuests.cend())
	{
		pQuestData = ItActiveQuest->second.pQuestData;

		//!!!can copy vars from event trigger and pass here for postprocessing and outcome mod
		// execute outcome script, pass outcome ID and reward for optional modification
		// apply reward
		// after possible outcome override:
		//if (!OutcomeID) return false;

		_ActiveQuests.erase(ItActiveQuest);
	}
	else
	{
		pQuestData = FindQuestData(ID);
	}

	n_assert(pQuestData);
	if (!pQuestData) return false;

	// To change the quest outcome, user must call ResetQuest. It never happens automatically
	// from quest settings and therefore it is safe from infinite loops in a queue.
	auto [_, Inserted] = _FinishedQuests.try_emplace(ID, OutcomeID);
	if (!Inserted) return false;

	auto ItOutcome = pQuestData->Outcomes.find(OutcomeID);

	// Finish dependent quests before auto-finishing children because here we can override outcome ID
	if (ItOutcome != pQuestData->Outcomes.cend())
		for (const auto [DependentID, DepOutcomeID] : ItOutcome->second.EndQuests)
			_ChangeQueue.emplace_back(DependentID, DepOutcomeID, Vars);

	// Automatically end child quests with the same outcome ID
	for (const auto& [ChildID, ActiveQuest] : _ActiveQuests)
		if (ActiveQuest.pQuestData->ParentID == ID)
			_ChangeQueue.emplace_back(ChildID, OutcomeID, Vars);

	// Activate dependent quests after finishing everything we wanted
	if (ItOutcome != pQuestData->Outcomes.cend())
		for (const CStrID DependentID : ItOutcome->second.StartQuests)
			_ChangeQueue.emplace_back(DependentID, CStrID::Empty, Vars);

	OnQuestCompleted(ID, OutcomeID);

	return true;
}
//---------------------------------------------------------------------

void CQuestManager::StartQuest(CStrID ID, PFlowVarStorage Vars)
{
	_ChangeQueue.emplace_back(ID, CStrID::Empty, std::move(Vars));
	ProcessQueue();
}
//---------------------------------------------------------------------

void CQuestManager::SetQuestOutcome(CStrID ID, CStrID OutcomeID, PFlowVarStorage Vars)
{
	_ChangeQueue.emplace_back(ID, OutcomeID, std::move(Vars));
	ProcessQueue();
}
//---------------------------------------------------------------------

void CQuestManager::ResetQuest(CStrID ID)
{
	// TODO: need OnReset script if active?
	_ActiveQuests.erase(ID);
	_FinishedQuests.erase(ID);
	_ChangeQueue.erase(std::remove_if(_ChangeQueue.begin(), _ChangeQueue.end(), [ID](const auto& Record) { return Record.QuestID == ID; }), _ChangeQueue.end());
}
//---------------------------------------------------------------------

std::pair<EQuestState, CStrID> CQuestManager::GetQuestState(CStrID ID) const
{
	auto ItActiveQuest = _ActiveQuests.find(ID);
	if (ItActiveQuest != _ActiveQuests.cend()) return { EQuestState::Active, {} };

	auto ItFinishedQuest = _FinishedQuests.find(ID);
	if (ItFinishedQuest != _FinishedQuests.cend()) return { EQuestState::Completed, ItFinishedQuest->second };

	return { EQuestState::NotStarted, {} };
}
//---------------------------------------------------------------------

// Quest system must preserve change order and linearize changes chaotically triggered by quest interdependencies
void CQuestManager::ProcessQueue()
{
	if (_IsInQueueProcessing) return;
	_IsInQueueProcessing = true;

	while (!_ChangeQueue.empty())
	{
		auto Record = std::move(_ChangeQueue.front());
		_ChangeQueue.pop_front();

		if (Record.OutcomeID)
			HandleQuestCompletion(Record.QuestID, Record.OutcomeID, std::move(Record.Vars));
		else
			HandleQuestStart(Record.QuestID, std::move(Record.Vars), false);
	}

	_IsInQueueProcessing = false;
}
//---------------------------------------------------------------------

}
