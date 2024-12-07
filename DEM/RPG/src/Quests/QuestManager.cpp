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

	// TODO: don't need to keep an asset loaded, maybe load here directly?
	// TODO: resource ID (CStrID) vs embedded string (std::string)?
	if (auto ScriptObject = _Session.GetScript(pQuestData->ScriptAssetID))
	{
		ItActiveQuest->second.FnOnStart = ScriptObject.get<sol::function>("OnStart");
		ItActiveQuest->second.FnOnComplete = ScriptObject.get<sol::function>("OnComplete");
	}

	// Do one time start logic, it should not run on loading already active quests
	if (!Loading)
	{
		// TODO: if Vars are mutable here, need to create copy in a script, because they may already be used in other quests
		Scripting::LuaCall(ItActiveQuest->second.FnOnStart, QuestVars.get());

		OnQuestStarted(ID);

		// Should not reset a quest from its handler
		const auto IsStillActive = IsQuestActive(ID);
		n_assert(IsStillActive);
		if (!IsStillActive) return false;

		// Activate/finish dependent quests before testing outcomes
		for (const auto [DependentID, DepOutcomeID] : pQuestData->EndQuests)
			EnqueueQuestCompletion(DependentID, DepOutcomeID, QuestVars);
		for (CStrID DependentID : pQuestData->StartQuests)
			EnqueueQuestStart(DependentID, QuestVars);
	}

	// Check outcomes in the order of priority.
	// Quest without outcomes can be only finished by another quest's EndQuests or by SetQuestOutcome call.
	for (const auto& [OutcomeID, OutcomeData] : pQuestData->Outcomes)
	{
		// Outcomes without a condition can be selected only by SetQuestOutcome call or outcome overriding in OnComplete script
		if (!OutcomeData.Condition.Type) continue;

		// Evaluate outcome condition immediately to catch already completed quests
		if (Flow::EvaluateCondition(OutcomeData.Condition, _Session, QuestVars.get()))
		{
			EnqueueQuestCompletion(ID, OutcomeID, QuestVars);
			return true;
		}
	}

	// Subscribe to events in reverse order to enforce desired call priority when subscribing to the same signal
	// FIXME: need subscription with priority! Now rely on CSignal LIFO behaviour.
	if (const auto* pConditions = _Session.FindFeature<Flow::CConditionRegistry>())
	{
		for (auto RIt = pQuestData->Outcomes.crbegin(); RIt != pQuestData->Outcomes.crend(); ++RIt)
		{
			const auto& [OutcomeID, OutcomeData] = *RIt;

			// Not satisfied condition will be re-tested on one of relevant events
			const auto& Cond = OutcomeData.Condition;
			if (auto* pCondition = pConditions->FindCondition(Cond.Type))
			{
				pCondition->SubscribeRelevantEvents(ItActiveQuest->second.Subs, { Cond, _Session, QuestVars.get() }, [this, ID, OutcomeID, &Cond](PFlowVarStorage EventVars)
				{
					if (Flow::EvaluateCondition(Cond, _Session, EventVars.get()))
					{
						// Don't unsubscribe from other events, give them a chance to trigger outcomes of a higher priority
						EnqueueQuestCompletion(ID, OutcomeID, std::move(EventVars));
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

		// FIXME: could not find a way to pass OutcomeID to Lua by ref, std::ref didn't help. Interferes with is_value_semantic_for_function?!
		//???maybe must remove is_value_semantic_for_function for non-const ref? make sure that https://github.com/ThePhD/sol2/issues/1335 will not return!
		{
			auto CompletionInfo = _Session.GetScriptState().create_table();
			CompletionInfo["OutcomeID"] = OutcomeID;
			Scripting::LuaCall(ItActiveQuest->second.FnOnComplete, Vars.get(), CompletionInfo/*, inout reward-from-balance?*/);
			OutcomeID = CompletionInfo["OutcomeID"];
		}

		// Completion could have been reverted in a handler
		if (!OutcomeID) return false;

		_ActiveQuests.erase(ItActiveQuest);

		// TODO: apply possibly-modified-reward-from-balance (only when finishing an active quest!)
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

	const auto* pOutcome = pQuestData->FindOutcome(OutcomeID);

	// Finish dependent quests before auto-finishing children because here we can override outcome ID
	if (pOutcome)
		for (const auto [DependentID, DepOutcomeID] : pOutcome->EndQuests)
			EnqueueQuestCompletion(DependentID, DepOutcomeID, Vars);

	// Automatically end child quests with the same outcome ID
	for (const auto& [ChildID, ActiveQuest] : _ActiveQuests)
		if (ActiveQuest.pQuestData->ParentID == ID)
			EnqueueQuestCompletion(ChildID, OutcomeID, Vars);

	// Activate dependent quests after finishing everything we wanted
	if (pOutcome)
		for (const CStrID DependentID : pOutcome->StartQuests)
			EnqueueQuestStart(DependentID, Vars);

	OnQuestCompleted(ID, OutcomeID);

	return true;
}
//---------------------------------------------------------------------

void CQuestManager::EnqueueQuestStart(CStrID ID, PFlowVarStorage Vars)
{
	_ChangeQueue.emplace_back(ID, CStrID::Empty, std::move(Vars));
}
//---------------------------------------------------------------------

void CQuestManager::EnqueueQuestCompletion(CStrID ID, CStrID OutcomeID, PFlowVarStorage Vars)
{
	// Outcomes are listed in the quest in the order of priority and can be overridden right in a queue.
	// This is required to correctly choose from multiple outcomes with conditions satisfied by the same event.
	auto It = std::remove_if(_ChangeQueue.begin(), _ChangeQueue.end(), [ID](const auto& Record) { return Record.QuestID == ID; });
	if (It == _ChangeQueue.cend())
	{
		_ChangeQueue.emplace_back(ID, OutcomeID, std::move(Vars));
	}
	else if (const auto* pQuestData = FindQuestData(ID))
	{
		if (pQuestData->FindOutcomeIndex(OutcomeID) < pQuestData->FindOutcomeIndex(It->OutcomeID))
		{
			It->OutcomeID = OutcomeID;
			It->Vars = std::move(Vars);
		}
	}
}
//---------------------------------------------------------------------

void CQuestManager::StartQuest(CStrID ID, PFlowVarStorage Vars)
{
	EnqueueQuestStart(ID, std::move(Vars));
	ProcessQueue();
}
//---------------------------------------------------------------------

void CQuestManager::SetQuestOutcome(CStrID ID, CStrID OutcomeID, PFlowVarStorage Vars)
{
	EnqueueQuestCompletion(ID, OutcomeID, std::move(Vars));
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
	if (IsQuestActive(ID)) return { EQuestState::Active, {} };
	if (auto OutcomeID = GetQuestOutcome(ID)) return { EQuestState::Completed, OutcomeID };
	return { EQuestState::NotStarted, {} };
}
//---------------------------------------------------------------------

CStrID CQuestManager::GetQuestOutcome(CStrID ID) const
{
	auto ItFinishedQuest = _FinishedQuests.find(ID);
	return (ItFinishedQuest != _FinishedQuests.cend()) ? ItFinishedQuest->second : CStrID::Empty;
}
//---------------------------------------------------------------------

// Quest system must preserve change order and linearize changes chaotically triggered by quest interdependencies
void CQuestManager::ProcessQueue()
{
	if (_IsInQueueProcessing || _ChangeQueue.empty()) return;
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
