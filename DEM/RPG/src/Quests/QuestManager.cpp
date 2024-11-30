#include "QuestManager.h"
#include <Quests/QuestData.h>
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
	// iterate root, must be ID -> Desc? or must be array! can then deserialize right to array, and then build ID map!
	//???CParams or CDataArray? how the file will look?

	// multiple objectives in one file, to simplify creating quests

	CQuestData Quest;
	DEM::ParamsFormat::Deserialize(Data::CData(Desc), Quest);
}
//---------------------------------------------------------------------

}
