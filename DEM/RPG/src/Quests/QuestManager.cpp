#include "QuestManager.h"
#include <Game/GameSession.h>
#include <Scripting/Flow/ConditionRegistry.h>
#include <Data/SerializeToParams.h>

namespace DEM::RPG
{

CQuestManager::CQuestManager(Game::CGameSession& Owner)
	: _Session(Owner)
{
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

bool CQuestManager::StartQuest(CStrID ID)
{
	// To start the quest again, user must call ResetQuest. It never happens automatically
	// from quest settings and therefore it is safe from infinite loops in a queue.
	auto ItFinishedQuest = _FinishedQuests.find(ID);
	if (ItFinishedQuest != _FinishedQuests.cend()) return false;

	auto* pQuestData = FindQuestData(ID);
	if (!pQuestData) return false;

	auto [ItActiveQuest, Inserted] = _ActiveQuests.try_emplace(ID);
	if (!Inserted) return false;

	auto& ActiveQuest = ItActiveQuest->second;
	ActiveQuest.pQuestData = pQuestData;

	// execute OnStart logic, either Lua or Flow
	//???what if Lua script will want to save state and to keep it until the quest end? store lua table? or forbid?
	//???need flow? could be useful, but flow here is not intended to last multiple frames!
	//???control by data type in the desc? treat PParams arg as a flow and string as a lua? what if want a file? CStrID and see extension?
	//???could be a special universal script type with complex deserialization logic?
	//???would want to have one script asset for multiple quest objectives, to reduce number of files? load to some table, internally q1 = {}; function q1:OnStart(...) and find by name?
	//???maybe general purpose script asset should simply listen OnQuestStarted and check ID, instead of personal script? personal is clearer!

	//???what if user finishes or resets this quest in the signal handler?!
	OnQuestStarted(ID);

	// Activate dependent quests before testing outcomes
	for (CStrID DependentID : pQuestData->StartQuests)
		_ChangeQueue.emplace_back(DependentID, CStrID::Empty);

	// Quest without outcomes can be only finished by its parent or by explicit SetQuestOutcome call
	for (const auto& [OutcomeID, OutcomeData] : pQuestData->Outcomes)
	{
		// Outcomes without a condition can be selected only by explicit SetQuestOutcome call
		if (!OutcomeData.Condition.Type) continue;

		const auto& Cond = OutcomeData.Condition;
		if (Flow::EvaluateCondition(Cond, _Session, nullptr))
		{
			_ChangeQueue.emplace_back(ID, OutcomeID);
		}
		else if (const auto* pConditions = _Session.FindFeature<Flow::CConditionRegistry>())
		{
			if (auto* pCondition = pConditions->FindCondition(Cond.Type))
			{
				pCondition->SubscribeRelevantEvents(ActiveQuest.Subs, { Cond, _Session, nullptr }, [this, ID, OutcomeID, &Cond](const Flow::CFlowVarStorage* pVars)
				{
					if (Flow::EvaluateCondition(Cond, _Session, pVars))
					{
						_ChangeQueue.emplace_back(ID, OutcomeID);
						ProcessQueue();
					}
				});
			}
		}
	}

	ProcessQueue();

	return true;
}
//---------------------------------------------------------------------

bool CQuestManager::SetQuestOutcome(CStrID ID, CStrID OutcomeID)
{
	// To change the quest outcome, user must call ResetQuest. It never happens automatically
	// from quest settings and therefore it is safe from infinite loops in a queue.
	auto [_, Inserted] = _FinishedQuests.try_emplace(ID, OutcomeID);
	if (!Inserted) return false;

	auto ItActiveQuest = _ActiveQuests.find(ID);
	if (ItActiveQuest != _ActiveQuests.cend())
	{
		// execute outcome script, pass outcome ID, pass reward for preprocessing
		// apply reward
		// start/end quests
	}

	OnQuestCompleted(ID, OutcomeID);

	return true;
}
//---------------------------------------------------------------------

// Quest system must preserve change order and linearize changes chaotically triggered by quest interdependencies
void CQuestManager::ProcessQueue()
{
	if (_IsInQueueProcessing) return;
	_IsInQueueProcessing = true;

	while (!_ChangeQueue.empty())
	{
		const auto [QuestID, OutcomeID] = _ChangeQueue.front();
		_ChangeQueue.pop_front();

		if (OutcomeID)
			SetQuestOutcome(QuestID, OutcomeID);
		else
			StartQuest(QuestID);
	}

	_IsInQueueProcessing = false;
}
//---------------------------------------------------------------------

}
