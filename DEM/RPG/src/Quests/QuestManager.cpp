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
	NOT_IMPLEMENTED;
	return false;
}
//---------------------------------------------------------------------

}
