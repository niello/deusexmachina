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
	auto* pQuestData = FindQuestData(ID);
	if (!pQuestData) return false;

	auto [ItActiveQuest, Inserted] = _ActiveQuests.try_emplace(ID);
	if (!Inserted) return false;

	auto& ActiveQuest = ItActiveQuest->second;
	ActiveQuest.pQuestData = pQuestData;

	// create active quest data structure, cache flow and/or script values
	// execute OnStart
	//   !!!if activates or deactivates other quests, must put them into a queue?
	// fire quest started event/signal

	// if no outcomes, finish immediately?
	// if outcome has no condition, finish immediately with this outcome? why to have this at all? assert on it?

	//???pass event params to vars? can vars be a part of a quest desc? need it really?
	Flow::CFlowVarStorage Vars;
	for (const auto& [OutcomeID, OutcomeData] : pQuestData->Outcomes)
	{
		const auto& Cond = OutcomeData.Condition;
		if (Flow::EvaluateCondition(Cond, _Session, Vars))
		{
			// immediately go to outcome OutcomeID //???or put it to queue?
		}
		else if (const auto* pConditions = _Session.FindFeature<Flow::CConditionRegistry>())
		{
			if (auto* pCondition = pConditions->FindCondition(Cond.Type))
			{
				pCondition->SubscribeRelevantEvents(ActiveQuest.Subs, { Cond, _Session, Vars }, [OutcomeID, &Cond]()
				{
					// re-evaluate condition and go to outcome if it is met
					::Sys::DbgOut(OutcomeID.CStr()); ::Sys::DbgOut("\n");
					::Sys::DbgOut(Cond.Type.CStr()); ::Sys::DbgOut("\n");
					NOT_IMPLEMENTED_MSG("QUEST RE-EVAL");
				});
			}
		}
	}

	//!!!if use queue, manager must correctly return quest state based on queued operations too!
	//can process queue right here in the end of the function recursively or in a loop with guard flag, or in CQuestManager::Update

	return true;
}
//---------------------------------------------------------------------

}
