#include "QuestManager.h"
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

	for (const auto& [OutcomeID, OutcomeData] : pQuestData->Outcomes)
	{
		// check all conditions, maybe immediately go to outcome //???or put it to queue?
		// if not, subscribe to condition events to check outcomes when something changes
	}

	//!!!if use queue, manager must correctlly return quest state based on queued operations too!
	//can process queue right here in the end of the function recursively or in a loop with guard flag, or in CQuestManager::Update

	NOT_IMPLEMENTED;
	return false;
}
//---------------------------------------------------------------------

}
