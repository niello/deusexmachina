#include "QuestManager.h"

#include <Scripting/ScriptServer.h>
#include <Scripting/ScriptObject.h>
#include <Events/EventManager.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Data/DataServer.h>
#include <IO/IOServer.h>

const nString StrQuests("Quests");
const nString StrUnderline("_");

//BEGIN_ATTRS_REGISTRATION(QuestManager)
//	RegisterStrID(QuestID, ReadWrite);
//	RegisterStrID(TaskID, ReadWrite);
//	RegisterInt(QStatus, ReadWrite);
//END_ATTRS_REGISTRATION

namespace Story
{
__ImplementClassNoFactory(Story::CQuestManager, Core::CRefCounted);
__ImplementSingleton(Story::CQuestManager);

using namespace Data;

CQuestManager::CQuestManager()
{
	__ConstructSingleton;

	IOSrv->SetAssign("quests", "game:quests");
	SUBSCRIBE_PEVENT(OnLoad, CQuestManager, OnLoad);
	SUBSCRIBE_PEVENT(OnSave, CQuestManager, OnSave);
}
//---------------------------------------------------------------------

CQuestManager::~CQuestManager()
{
	UNSUBSCRIBE_EVENT(OnLoad);
	UNSUBSCRIBE_EVENT(OnSave);

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
				DeletedScriptObjects.Append(TasksToDelete[i]->ScriptObj->GetFullName());
				TasksToDelete[i]->ScriptObj = NULL;
			}
		}
		TasksToDelete.Clear();
	}
}
//---------------------------------------------------------------------

bool CQuestManager::LoadQuest(CStrID QuestID, CStrID* OutStartingTaskID)
{
	PParams QuestDesc = DataSrv->LoadPRM(nString("quests:") + QuestID.CStr() + "/_Quest.prm", false);
	if (!QuestDesc.IsValid()) FAIL;

	Ptr<CQuest> Quest = n_new(CQuest);
	Quest->Name = QuestDesc->Get<nString>(CStrID("Name"), "<No quest name>");
	Quest->Description = QuestDesc->Get<nString>(CStrID("Desc"), "<No quest desc>");

	const CParams& Tasks = *QuestDesc->Get<PParams>(CStrID("Tasks"));
	for (int i = 0; i < Tasks.GetCount(); i++)
	{
		const CParam& TaskPrm = Tasks[i];
		const CParams& TaskDesc = *TaskPrm.GetValue<PParams>();

		Ptr<CTask> NewTask = n_new(CTask);
		NewTask->Name = TaskDesc.Get<nString>(CStrID("Name"), "<No task name>");
		NewTask->Description = TaskDesc.Get<nString>(CStrID("Desc"), "<No task desc>");

		CQuest::CTaskRec NewTR;
		NewTR.Task = NewTask;
		NewTR.Status = CQuest::No;
		Quest->Tasks.Add(TaskPrm.GetName(), NewTR);
	}

	//???or store StartingTask in CQuest and allow setting it later, not only on task loading? really need?
	if (OutStartingTaskID)
		*OutStartingTaskID = CStrID(QuestDesc->Get<nString>(CStrID("StartingTask"), "").CStr());

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
		PParams P = n_new(CParams);
		P->Set(CStrID("IsTask"), false);
		P->Set(CStrID("Status"), (int)CQuest::Opened);
		P->Set(CStrID("Name"), Quest->Name);
		P->Set(CStrID("Desc"), Quest->Description);
		EventMgr->FireEvent(CStrID("OnQuestStatusChanged"), P, EV_ASYNC);
		//add Story::CJournal record or it will receive event too
	}
	else
	{
		Quest = Quests.ValueAt(Idx).Quest;
		n_assert(Quests.ValueAt(Idx).Status == CQuest::Opened);
	}

	if (TaskID == CStrID::Empty) 
		n_error("No quest task specified either explicitly (as an argument) or implicitly (in the quest description)");
	
	CQuest::CTaskRec& Task = Quest->Tasks[TaskID];
	Task.Status = CQuest::Opened;

	//!!!refactor params!
	PParams P = n_new(CParams);
	P->Set(CStrID("IsTask"), true);
	P->Set(CStrID("Status"), (int)CQuest::Opened);
	P->Set(CStrID("Name"), Task.Task->Name);
	P->Set(CStrID("Desc"), Task.Task->Description);
	EventMgr->FireEvent(CStrID("OnQuestStatusChanged"), P, EV_ASYNC);
	//add Story::CJournal record or it will receive event too

#ifdef _DEBUG
	n_printf("TASK \"%s\" started. %s\n", Task.Task->Name.CStr(), Task.Task->Description.CStr());
#endif

	//!!!there was some benefit to create script obj before event (close task before notifying it's opened?)
	//remember it!

	// Run script at last cause we want to have up-to-date current task status
	// This way we can immediately close the task we're starting now by it's own script
	nString TaskScriptFile = nString("quests:") + QuestID.CStr() + "/" + TaskID.CStr() + ".lua";
	if (IOSrv->FileExists(TaskScriptFile)) //???is optimal?
	{
		nString Name = nString(QuestID.CStr()) + StrUnderline + TaskID.CStr();
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
				PParams P = n_new(CParams);
				P->Set(CStrID("IsTask"), true);
				P->Set(CStrID("Status"), (int)Status);
				P->Set(CStrID("Name"), Task.Task->Name);
				EventMgr->FireEvent(CStrID("OnQuestStatusChanged"), P, EV_ASYNC);
				//add Story::CJournal record or it will receive event too

				// Don't delete task, we need its name & desc for journal
				TasksToDelete.Append(Task.Task);
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
		//QuestsToDelete.Append(Quest);
		//Quests.ValueAt(Idx).Quest = NULL;
		Quests.ValueAt(Idx).Status = Status;

		//!!!refactor params!
		PParams P = n_new(CParams);
		P->Set(CStrID("IsTask"), false);
		P->Set(CStrID("Status"), (int)Status);
		P->Set(CStrID("Name"), Quests.ValueAt(Idx).Quest->Name);
		EventMgr->FireEvent(CStrID("OnQuestStatusChanged"), P, EV_ASYNC);
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
			PParams P = n_new(CParams);
			P->Set(CStrID("IsTask"), true);
			P->Set(CStrID("Status"), (int)Status);
			P->Set(CStrID("Name"), Task.Task->Name);
			EventMgr->FireEvent(CStrID("OnQuestStatusChanged"), P, EV_ASYNC);
			//add Story::CJournal record or it will receive event too

			// Don't delete task, we need its name & desc for journal
			TasksToDelete.Append(Task.Task);
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

bool CQuestManager::OnLoad(const Events::CEventBase& Event)
{
	/*
	DB::CDatabase* pDB = (DB::CDatabase*)((const Events::CEvent&)Event).Params->Get<PVOID>(CStrID("DB"));

	QuestsToDelete.Clear();
	TasksToDelete.Clear();
	DeletedScriptObjects.Clear();
	
	int TblIdx = pDB->FindTableIndex(StrQuests);
	if (TblIdx == INVALID_INDEX)
	{
		Quests.Clear();
		OK;
	}

	for (int i = 0; i < Quests.GetCount(); i++)
	{
		CQuestRec& QuestRec = Quests.ValueAt(i);
		QuestRec.Status = CQuest::No;
		nDictionary<CStrID, CQuest::CTaskRec>& Tasks = QuestRec.Quest->Tasks;
		for (int j = 0; j < Tasks.GetCount(); j++)
		{
			CQuest::CTaskRec& TaskRec = Tasks.ValueAt(j);
			TaskRec.Status = CQuest::No;
			TaskRec.Task->ScriptObj = NULL;
		}
	}

	DB::PDataset DS = pDB->GetTable(TblIdx)->CreateDataset();
	DS->AddColumnsFromTable();
	DS->PerformQuery();
	
	for (int i = 0; i < DS->GetValueTable()->GetRowCount(); i++)
	{
		DS->SetRowIndex(i);

		CStrID QuestID = DS->Get<CStrID>(Attr::QuestID);

		CQuestRec* QuestRec;

		int Idx = Quests.FindIndex(QuestID);
		if (Idx == INVALID_INDEX)
		{
			if (!LoadQuest(QuestID)) FAIL;
			QuestRec = &Quests[QuestID];
		}
		else QuestRec = &Quests.ValueAt(Idx);

		CStrID TaskID = DS->Get<CStrID>(Attr::TaskID);
		if (TaskID.IsValid())
		{
			CQuest::CTaskRec& TaskRec = QuestRec->Quest->Tasks[TaskID];
			TaskRec.Status = (CQuest::EStatus)DS->Get<int>(Attr::QStatus);
			if (TaskRec.Status == CQuest::Opened)
			{
				nString TaskScriptFile = nString("quests:") + QuestID.CStr() + "/" + TaskID.CStr() + ".lua";
				if (IOSrv->FileExists(TaskScriptFile)) //???is optimal?
				{
					nString Name = nString(QuestID.CStr()) + StrUnderline + TaskID.CStr();
					Name.ReplaceChars("/", '_');
					TaskRec.Task->ScriptObj = n_new(Scripting::CScriptObject(Name.CStr(), StrQuests.CStr()));
					TaskRec.Task->ScriptObj->Init();
					TaskRec.Task->ScriptObj->LoadScriptFile(TaskScriptFile);
					TaskRec.Task->ScriptObj->LoadFields(pDB);
				}
			}
		}
		else QuestRec->Status = (CQuest::EStatus)DS->Get<int>(Attr::QStatus);
	}

	for (int i = Quests.GetCount() - 1; i >= 0; i--)
		if (Quests.ValueAt(i).Status == CQuest::No)
			Quests.EraseAt(i);
*/
	OK;
}
//---------------------------------------------------------------------

bool CQuestManager::OnSave(const Events::CEventBase& Event)
{
/*
	DB::CDatabase* pDB = (DB::CDatabase*)((const Events::CEvent&)Event).Params->Get<PVOID>(CStrID("DB"));

	//????!!!!move to script server & make general policy to chande LuaObjects table (only OnSave)?!
	//!!!use WHERE ... IN instead of 100500 queries!
	for (int i = 0; i < DeletedScriptObjects.GetCount(); i++)
		Scripting::CScriptObject::ClearFieldsDeffered(pDB, DeletedScriptObjects[i]);
	DeletedScriptObjects.Clear();

	DB::PTable Tbl;
	int TblIdx = pDB->FindTableIndex(StrQuests);
	if (TblIdx == INVALID_INDEX)
	{
		Tbl = DB::CTable::CreateInstance();
		Tbl->SetName(StrQuests);
		Tbl->AddColumn(DB::CColumn(Attr::QuestID, DB::CColumn::Primary));
		Tbl->AddColumn(DB::CColumn(Attr::TaskID, DB::CColumn::Primary));
		Tbl->AddColumn(Attr::QStatus);
		pDB->AddTable(Tbl);
	}
	else
	{
		Tbl = pDB->GetTable(TblIdx);
		Tbl->Truncate();
	}

	DB::PDataset DS = Tbl->CreateDataset();
	DS->AddColumnsFromTable();

	for (int i = 0; i < Quests.GetCount(); i++)
	{
		CQuestRec& QuestRec = Quests.ValueAt(i);
		if (QuestRec.Status != CQuest::No)
		{
			DS->AddRow();
			DS->Set<CStrID>(Attr::QuestID, Quests.KeyAt(i));
			DS->Set<int>(Attr::QStatus, (int)QuestRec.Status);
			if (QuestRec.Status == CQuest::Opened)
			{
				nDictionary<CStrID, CQuest::CTaskRec>& Tasks = QuestRec.Quest->Tasks;
				for (int j = 0; j < Tasks.GetCount(); j++)
				{
					CQuest::CTaskRec& TaskRec = Tasks.ValueAt(j);
					if (TaskRec.Status != CQuest::No)
					{
						DS->AddRow();
						DS->Set<CStrID>(Attr::QuestID, Quests.KeyAt(i));
						DS->Set<CStrID>(Attr::TaskID, Tasks.KeyAt(j));
						DS->Set<int>(Attr::QStatus, (int)TaskRec.Status);
						if (TaskRec.Status == CQuest::Opened && TaskRec.Task->ScriptObj.IsValid())
							TaskRec.Task->ScriptObj->SaveFields(pDB);
					}
				}
			}
		}
	}

	DS->CommitChanges();
*/
	OK;
}
//---------------------------------------------------------------------

} //namespace Story