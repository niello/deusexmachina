#include "QuestManager.h"

#include <Scripting/ScriptServer.h>
#include <Scripting/ScriptObject.h>
#include <Events/EventServer.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Data/DataServer.h>
#include <IO/IOServer.h>

const CString StrQuests("Quests");
const CString StrUnderline("_");

namespace Story
{
__ImplementClassNoFactory(Story::CQuestManager, Core::CRefCounted);
__ImplementSingleton(Story::CQuestManager);

CQuestManager::CQuestManager()
{
	__ConstructSingleton;

	SUBSCRIBE_PEVENT(OnGameDescLoaded, CQuestManager, OnGameDescLoaded);
	SUBSCRIBE_PEVENT(OnGameSaving, CQuestManager, OnGameSaving);
}
//---------------------------------------------------------------------

CQuestManager::~CQuestManager()
{
	UNSUBSCRIBE_EVENT(OnGameDescLoaded);
	UNSUBSCRIBE_EVENT(OnGameSaving);

	__DestructSingleton;
}
//---------------------------------------------------------------------

void CQuestManager::Trigger()
{
	if (QuestsToDelete.GetCount()) QuestsToDelete.Clear();

	//!!!tasks aren't deleted now
	if (TasksToDelete.GetCount())
	{
		for (int i = 0; i < TasksToDelete.GetCount(); i++)
		{
			if (TasksToDelete[i]->ScriptObj.IsValid())
			{
				DeletedScriptObjects.Add(TasksToDelete[i]->ScriptObj->GetFullName());
				TasksToDelete[i]->ScriptObj = NULL;
			}
		}
		TasksToDelete.Clear();
	}
}
//---------------------------------------------------------------------

bool CQuestManager::LoadQuest(CStrID QuestID, CStrID* OutStartingTaskID)
{
	Data::PParams QuestDesc = DataSrv->LoadPRM(CString("Quests:") + QuestID.CStr() + "/_Quest.prm", false);
	if (!QuestDesc.IsValid()) FAIL;

	Ptr<CQuest> Quest = n_new(CQuest);
	Quest->Name = QuestDesc->Get<CString>(CStrID("Name"), "<No quest name>");
	Quest->Description = QuestDesc->Get<CString>(CStrID("Desc"), "<No quest desc>");

	const Data::CParams& Tasks = *QuestDesc->Get<Data::PParams>(CStrID("Tasks"));
	for (int i = 0; i < Tasks.GetCount(); i++)
	{
		const Data::CParam& TaskPrm = Tasks[i];
		const Data::CParams& TaskDesc = *TaskPrm.GetValue<Data::PParams>();

		Ptr<CTask> NewTask = n_new(CTask);
		NewTask->Name = TaskDesc.Get<CString>(CStrID("Name"), "<No task name>");
		NewTask->Description = TaskDesc.Get<CString>(CStrID("Desc"), "<No task desc>");

		CQuest::CTaskRec NewTR;
		NewTR.Task = NewTask;
		NewTR.Status = CQuest::No;
		Quest->Tasks.Add(TaskPrm.GetName(), NewTR);
	}

	//???or store StartingTask in CQuest and allow setting it later, not only on task loading? really need?
	if (OutStartingTaskID)
		*OutStartingTaskID = QuestDesc->Get<CStrID>(CStrID("StartingTask"), CStrID::Empty);

	CQuestRec Rec;
	Rec.Quest = Quest;
	Rec.Status = CQuest::No;
	Quests.Add(QuestID, Rec);

	OK;
}
//---------------------------------------------------------------------

bool CQuestManager::StartQuest(CStrID QuestID, CStrID TaskID)
{
	if (QuestID == CStrID::Empty || GetQuestStatus(QuestID, TaskID) != CQuest::No) FAIL;

	n_printf("QuestMgr: starting quest %s, task %s\n", QuestID.CStr(), TaskID.CStr());

	Ptr<CQuest> Quest;

	int Idx = Quests.FindIndex(QuestID);
	if (Idx == INVALID_INDEX) //???or found & status = No?
	{
		if (!LoadQuest(QuestID, (TaskID == CStrID::Empty) ? &TaskID : NULL)) FAIL;
		CQuestRec& NewRec = Quests[QuestID];
		Quest = NewRec.Quest;
		NewRec.Status = CQuest::Opened;

		//!!!refactor params!
		Data::PParams P = n_new(Data::CParams);
		P->Set(CStrID("IsTask"), false);
		P->Set(CStrID("Status"), (int)CQuest::Opened);
		P->Set(CStrID("Name"), Quest->Name);
		P->Set(CStrID("Desc"), Quest->Description);
		EventSrv->FireEvent(CStrID("OnQuestStatusChanged"), P, EV_ASYNC);
		//add Story::CJournal record or it will receive event too
	}
	else
	{
		Quest = Quests.ValueAt(Idx).Quest;
		n_assert(Quests.ValueAt(Idx).Status == CQuest::Opened);
	}

	if (TaskID == CStrID::Empty) 
		Core::Error("No quest task specified either explicitly (as an argument) or implicitly (in the quest description)");
	
	CQuest::CTaskRec& Task = Quest->Tasks[TaskID];
	Task.Status = CQuest::Opened;

	//!!!refactor params!
	Data::PParams P = n_new(Data::CParams);
	P->Set(CStrID("IsTask"), true);
	P->Set(CStrID("Status"), (int)CQuest::Opened);
	P->Set(CStrID("Name"), Task.Task->Name);
	P->Set(CStrID("Desc"), Task.Task->Description);
	EventSrv->FireEvent(CStrID("OnQuestStatusChanged"), P, EV_ASYNC);
	//add Story::CJournal record or it will receive event too

#ifdef _DEBUG
	n_printf("TASK \"%s\" started. %s\n", Task.Task->Name.CStr(), Task.Task->Description.CStr());
#endif

	//!!!there was some benefit to create script obj before event (close task before notifying it's opened?)
	//remember it!

	// Run script at last cause we want to have up-to-date current task status
	// This way we can immediately close the task we're starting now by it's own script
	CString TaskScriptFile = CString("Quests:") + QuestID.CStr() + "/" + TaskID.CStr() + ".lua";
	if (IOSrv->FileExists(TaskScriptFile)) //???is optimal?
	{
		CString Name = CString(QuestID.CStr()) + StrUnderline + TaskID.CStr();
		Name.ReplaceChars("/", '_');
		Task.Task->ScriptObj = n_new(Scripting::CScriptObject(Name.CStr(), StrQuests.CStr()));
		Task.Task->ScriptObj->Init(); // No special class
		Task.Task->ScriptObj->LoadScriptFile(TaskScriptFile);
	}

	OK;
}
//---------------------------------------------------------------------

//!!!CODE DUPLICATIONS!
bool CQuestManager::CloseQuest(CStrID QuestID, CStrID TaskID, bool Success)
{
	int Idx = Quests.FindIndex(QuestID);
	if (Idx == INVALID_INDEX) FAIL;

	Ptr<CQuest> Quest = Quests.ValueAt(Idx).Quest;

	if (Quests.ValueAt(Idx).Status != CQuest::Opened) FAIL;

	CQuest::EStatus Status = (Success) ? CQuest::Done : CQuest::Failed;

	if (TaskID == CStrID::Empty)
	{
		for (int i = 0; i < Quest->Tasks.GetCount(); i++)
		{
			CQuest::CTaskRec& Task = Quest->Tasks.ValueAt(i);
			if (Task.Status == CQuest::Opened)
			{
				//???move to CloseTask()? see below.

				//!!!refactor params!
				Data::PParams P = n_new(Data::CParams);
				P->Set(CStrID("IsTask"), true);
				P->Set(CStrID("Status"), (int)Status);
				P->Set(CStrID("Name"), Task.Task->Name);
				EventSrv->FireEvent(CStrID("OnQuestStatusChanged"), P, EV_ASYNC);
				//add Story::CJournal record or it will receive event too

				// Don't delete task, we need its name & desc for journal
				TasksToDelete.Add(Task.Task);
				//Task.Task = NULL;
				Task.Status = Status;
			}
		}

#ifdef _DEBUG
		CQuest* Quest = Quests.ValueAt(Idx).Quest;
		n_printf("QUEST \"%s\" closed %s.\n",
			Quest->Name.CStr(),
			Success ? "successfully" : "with failure");
#endif

		// Do not unload to access to tasks' Status now
		//QuestsToDelete.Add(Quest);
		//Quests.ValueAt(Idx).Quest = NULL;
		Quests.ValueAt(Idx).Status = Status;

		//!!!refactor params!
		Data::PParams P = n_new(Data::CParams);
		P->Set(CStrID("IsTask"), false);
		P->Set(CStrID("Status"), (int)Status);
		P->Set(CStrID("Name"), Quests.ValueAt(Idx).Quest->Name);
		EventSrv->FireEvent(CStrID("OnQuestStatusChanged"), P, EV_ASYNC);
		//add Story::CJournal record or it will receive event too

		OK;
	}
	else
	{
		// complete & unload task
		Idx = Quest->Tasks.FindIndex(TaskID);
		if (Idx != INVALID_INDEX) //???&& status == opened?
		{
			CQuest::CTaskRec& Task = Quest->Tasks.ValueAt(Idx);

			if (Task.Status != CQuest::Opened) FAIL;

#ifdef _DEBUG
			n_printf("TASK \"%s\" closed %s.\n", Task.Task->Name.CStr(),
				Success ? "successfully" : "with failure");
#endif

			//!!!refactor params!
			Data::PParams P = n_new(Data::CParams);
			P->Set(CStrID("IsTask"), true);
			P->Set(CStrID("Status"), (int)Status);
			P->Set(CStrID("Name"), Task.Task->Name);
			EventSrv->FireEvent(CStrID("OnQuestStatusChanged"), P, EV_ASYNC);
			//add Story::CJournal record or it will receive event too

			// Don't delete task, we need its name & desc for journal
			TasksToDelete.Add(Task.Task);
			//Task.Task = NULL;
			Task.Status = Status;

			OK;
		}
	}
	
	FAIL;
}
//---------------------------------------------------------------------

CQuest::EStatus CQuestManager::GetQuestStatus(CStrID QuestID, CStrID TaskID)
{
	int Idx = Quests.FindIndex(QuestID);
	if (Idx == INVALID_INDEX) return CQuest::No;
	if (TaskID == CStrID::Empty) //|| Quests.ValueAt(Idx).Status != CQuest::Opened)
		return Quests.ValueAt(Idx).Status;
	else
	{
		Ptr<CQuest> Quest = Quests.ValueAt(Idx).Quest;
		if (Quest.IsValid())
		{
			Idx = Quest->Tasks.FindIndex(TaskID);
			if (Idx != INVALID_INDEX) return Quest->Tasks.ValueAt(Idx).Status;
			else if (Quests.ValueAt(Idx).Status != CQuest::Opened) return Quests.ValueAt(Idx).Status;
		}
	}
	return CQuest::No;
}
//---------------------------------------------------------------------

bool CQuestManager::OnGameDescLoaded(const Events::CEventBase& Event)
{
	QuestsToDelete.Clear();
	TasksToDelete.Clear();

	Data::PParams GameDesc = ((const Events::CEvent&)Event).Params;
	Data::PDataArray SGQuests;
	if (!GameDesc->Get(SGQuests, CStrID("Quests")) || !SGQuests->GetCount())
	{
		Quests.Clear();
		OK;
	}

	// Reset all loaded quests instead of clearing array to avoid reloading from descs
	for (int i = 0; i < Quests.GetCount(); i++)
	{
		CQuestRec& QuestRec = Quests.ValueAt(i);
		QuestRec.Status = CQuest::No;
		CDict<CStrID, CQuest::CTaskRec>& Tasks = QuestRec.Quest->Tasks;
		for (int j = 0; j < Tasks.GetCount(); j++)
		{
			CQuest::CTaskRec& TaskRec = Tasks.ValueAt(j);
			TaskRec.Status = CQuest::No;
			TaskRec.Task->ScriptObj = NULL;
		}
	}

	for (int i = 0; i < SGQuests->GetCount(); ++i)
	{
		Data::PParams SGQuest = SGQuests->Get(i);

		CStrID QuestID = SGQuest->Get<CStrID>(CStrID("ID"));

		CQuestRec* QuestRec;

		int Idx = Quests.FindIndex(QuestID);
		if (Idx == INVALID_INDEX)
		{
			if (!LoadQuest(QuestID)) FAIL;
			QuestRec = &Quests[QuestID];
		}
		else QuestRec = &Quests.ValueAt(Idx);

		QuestRec->Status = (CQuest::EStatus)SGQuest->Get<int>(CStrID("Status"));

		Data::PParams SGTasks;
		if (SGQuest->Get(SGTasks, CStrID("Tasks")))
		{
			for (int j = 0; j < SGTasks->GetCount(); ++j)
			{
				CStrID TaskID = SGTasks->Get(j).GetName();
				CQuest::CTaskRec& TaskRec = QuestRec->Quest->Tasks[TaskID];
				TaskRec.Status = (CQuest::EStatus)SGTasks->Get<int>(j);
				if (TaskRec.Status == CQuest::Opened)
				{
					CString TaskScriptFile = CString("Quests:") + QuestID.CStr() + "/" + TaskID.CStr() + ".lua";
					if (IOSrv->FileExists(TaskScriptFile)) //???is optimal?
					{
						CString Name = CString(QuestID.CStr()) + StrUnderline + TaskID.CStr();
						Name.ReplaceChars("/", '_');
						TaskRec.Task->ScriptObj = n_new(Scripting::CScriptObject(Name.CStr(), StrQuests.CStr()));
						TaskRec.Task->ScriptObj->Init();
						TaskRec.Task->ScriptObj->LoadScriptFile(TaskScriptFile);
					}
				}
			}
		}
	}

	// Remove quests that weren't loaded
	for (int i = Quests.GetCount() - 1; i >= 0; --i)
		if (Quests.ValueAt(i).Status == CQuest::No)
			Quests.RemoveAt(i);

	OK;
}
//---------------------------------------------------------------------

bool CQuestManager::OnGameSaving(const Events::CEventBase& Event)
{
	Data::PParams SGCommon = ((const Events::CEvent&)Event).Params;

	Data::PDataArray SGQuests = n_new(Data::CDataArray);
	for (int i = 0; i < Quests.GetCount(); ++i)
	{
		CQuestRec& QuestRec = Quests.ValueAt(i);
		if (QuestRec.Status == CQuest::No) continue;

		Data::PParams SGQuest = n_new(Data::CParams);
		SGQuest->Set(CStrID("ID"), Quests.KeyAt(i));
		SGQuest->Set(CStrID("Status"), (int)QuestRec.Status);
		SGQuests->Add(SGQuest);

		if (QuestRec.Status != CQuest::Opened) continue;

		Data::PParams SGTasks = n_new(Data::CParams);
		CDict<CStrID, CQuest::CTaskRec>& Tasks = QuestRec.Quest->Tasks;
		for (int j = 0; j < Tasks.GetCount(); j++)
		{
			CQuest::CTaskRec& TaskRec = Tasks.ValueAt(j);
			if (TaskRec.Status == CQuest::No) continue;
			SGTasks->Set(Tasks.KeyAt(j), (int)TaskRec.Status);
		}
		if (SGTasks->GetCount()) SGQuest->Set(CStrID("Tasks"), SGTasks);
	}

	if (SGQuests->GetCount()) SGCommon->Set(CStrID("Quests"), SGQuests);

	OK;
}
//---------------------------------------------------------------------

}