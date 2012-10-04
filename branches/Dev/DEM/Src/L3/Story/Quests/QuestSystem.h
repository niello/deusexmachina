#pragma once
#ifndef __DEM_L3_QUEST_SYSTEM_H__
#define __DEM_L3_QUEST_SYSTEM_H__

#include "Quest.h"
#include <Events/Events.h>
#include <db/AttrID.h>

// Quest system manages current player (character) tasks and their flow (completion, failure,
// opening new tasks etc)

//???do CQuest & CTask need refcount or they can be rewritten as simple structs?

namespace Attr
{
	DeclareStrID(QuestID);
	DeclareStrID(TaskID);
	DeclareInt(QStatus);
}

namespace Story
{

#define QuestSys Story::CQuestSystem::Instance()

class CQuest;

class CQuestSystem: public Core::CRefCounted //???Game::CManager?
{
	DeclareRTTI;
	DeclareFactory(CQuestSystem);

private:

	static CQuestSystem* Singleton;

	struct CQuestRec
	{
		Ptr<CQuest>		Quest;
		CQuest::EStatus	Status;
	};
	nDictionary<CStrID, CQuestRec> Quests;

	nArray<Ptr<CQuest>>	QuestsToDelete;
	nArray<Ptr<CTask>>	TasksToDelete;
	nArray<nString>		DeletedScriptObjects;

	bool LoadQuest(CStrID QuestID, CStrID* OutStartingTaskID = NULL);
	bool CloseQuest(CStrID QuestID, CStrID TaskID, bool Success);

	DECLARE_EVENT_HANDLER(OnLoad, OnLoad);
	DECLARE_EVENT_HANDLER(OnSave, OnSave);

public:

	CQuestSystem();
	~CQuestSystem();

	static CQuestSystem* Instance() { n_assert(Singleton); return Singleton; }

	void			Trigger();

	bool			StartQuest(CStrID QuestID, CStrID TaskID = CStrID::Empty);
	bool			CompleteQuest(CStrID QuestID, CStrID TaskID = CStrID::Empty);
	bool			FailQuest(CStrID QuestID, CStrID TaskID = CStrID::Empty);
	CQuest::EStatus	GetQuestStatus(CStrID QuestID, CStrID TaskID = CStrID::Empty);

	// Save, Load
};

RegisterFactory(CQuestSystem);

inline bool CQuestSystem::CompleteQuest(CStrID QuestID, CStrID TaskID)
{
	n_printf("QuestSys: completed quest %s, task %s\n", QuestID.CStr(), TaskID.CStr());
	return CloseQuest(QuestID, TaskID, true);
}
//---------------------------------------------------------------------

inline bool CQuestSystem::FailQuest(CStrID QuestID, CStrID TaskID)
{
	n_printf("QuestSys: failed quest %s, task %s\n", QuestID.CStr(), TaskID.CStr());
	return CloseQuest(QuestID, TaskID, false);
}
//---------------------------------------------------------------------

}

#endif
