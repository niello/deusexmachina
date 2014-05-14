#pragma once
#ifndef __DEM_L3_QUEST_SYSTEM_H__
#define __DEM_L3_QUEST_SYSTEM_H__

#include <Data/Singleton.h>
#include <Quests/Quest.h>
#include <Events/EventsFwd.h>

// Quest system manages current player (character) tasks and their flow (completion, failure,
// opening new tasks etc)

//???do CQuest & CTask need refcount or they can be rewritten as simple structs?

namespace Story
{
#define QuestMgr Story::CQuestManager::Instance()

class CQuestManager: public Core::CObject //???Game::CManager?
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CQuestManager);

private:

	struct CQuestRec
	{
		Ptr<CQuest>		Quest;
		CQuest::EStatus	Status;
	};
	CDict<CStrID, CQuestRec> Quests;

	CArray<Ptr<CQuest>>	QuestsToDelete;
	CArray<Ptr<CTask>>	TasksToDelete;
	CArray<CString>		DeletedScriptObjects;

	bool LoadQuest(CStrID QuestID, CStrID* OutStartingTaskID = NULL);
	bool CloseQuest(CStrID QuestID, CStrID TaskID, bool Success);

	DECLARE_EVENT_HANDLER(OnGameDescLoaded, OnGameDescLoaded);
	DECLARE_EVENT_HANDLER(OnGameSaving, OnGameSaving);

public:

	CQuestManager();
	~CQuestManager();

	void			Trigger();

	bool			StartQuest(CStrID QuestID, CStrID TaskID = CStrID::Empty);
	bool			CompleteQuest(CStrID QuestID, CStrID TaskID = CStrID::Empty);
	bool			FailQuest(CStrID QuestID, CStrID TaskID = CStrID::Empty);
	CQuest::EStatus	GetQuestStatus(CStrID QuestID, CStrID TaskID = CStrID::Empty);

	// Save, Load
};

inline bool CQuestManager::CompleteQuest(CStrID QuestID, CStrID TaskID)
{
	Core::Log("QuestMgr: completed quest %s, task %s\n", QuestID.CStr(), TaskID.CStr());
	return CloseQuest(QuestID, TaskID, true);
}
//---------------------------------------------------------------------

inline bool CQuestManager::FailQuest(CStrID QuestID, CStrID TaskID)
{
	Core::Log("QuestMgr: failed quest %s, task %s\n", QuestID.CStr(), TaskID.CStr());
	return CloseQuest(QuestID, TaskID, false);
}
//---------------------------------------------------------------------

}

#endif
