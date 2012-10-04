#include "PropUIControl.h"

#include <Game/Entity.h>
#include <AI/Events/QueueTask.h>
#include <AI/Prop/PropActorBrain.h>
#include <AI/Prop/PropSmartObject.h>
#include <AI/SmartObj/Tasks/TaskUseSmartObj.h>
#include <Scripting/Prop/PropScriptable.h>
#include <Scripting/EventHandlerScript.h>
#include <Data/DataServer.h>
#include <Events/EventManager.h>
#include <Loading/EntityFactory.h>
#include <DB/DBServer.h>

namespace Attr
{
	DefineString(IAODesc);
	DefineString(Name);
};

BEGIN_ATTRS_REGISTRATION(PropUIControl)
	RegisterString(IAODesc, ReadOnly);
	RegisterString(Name, ReadOnly);
END_ATTRS_REGISTRATION

namespace Properties
{
ImplementRTTI(Properties::CPropUIControl, Game::CProperty);
ImplementFactory(Properties::CPropUIControl);
ImplementPropertyStorage(CPropUIControl, 128);
RegisterProperty(CPropUIControl);

using namespace Data;

void CPropUIControl::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	Attrs.Append(Attr::IAODesc);
	Attrs.Append(Attr::Name);
}
//---------------------------------------------------------------------

void CPropUIControl::Activate()
{
	Game::CProperty::Activate();

	UIName = GetEntity()->Get<nString>(Attr::Name);

	PParams Desc;
	const nString& IAODesc = GetEntity()->Get<nString>(Attr::IAODesc);
	if (IAODesc.IsValid()) Desc = DataSrv->LoadPRM(nString("iao:") + IAODesc + ".prm");

	if (Desc.isvalid())
	{
		if (UIName.IsEmpty()) UIName = Desc->Get<nString>(CStrID("UIName"), NULL);
		AutoAddSmartObjActions = Desc->Get<bool>(CStrID("AutoAddSmartObjActions"), true);

		//???read priorities for actions? or all through scripts?

		//???read custom commands or add through script?
		
		SOActionNames = Desc->Get<PParams>(CStrID("SmartObjActionNames"), NULL);
	}
	else AutoAddSmartObjActions = true;
	
	if (AutoAddSmartObjActions)
	{
		PROP_SUBSCRIBE_PEVENT(OnPropsActivated, CPropUIControl, OnPropsActivated);
		PROP_SUBSCRIBE_PEVENT(OnSOActionAvailabile, CPropUIControl, OnSOActionAvailabile);
	}
	PROP_SUBSCRIBE_PEVENT(ExposeSI, CPropUIControl, ExposeSI);
	PROP_SUBSCRIBE_PEVENT(ObjMouseOver, CPropUIControl, OnObjMouseOver);
	PROP_SUBSCRIBE_PEVENT(OverrideUIName, CPropUIControl, OverrideUIName);
}
//---------------------------------------------------------------------

void CPropUIControl::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnPropsActivated);
	UNSUBSCRIBE_EVENT(ExposeSI);
	UNSUBSCRIBE_EVENT(ObjMouseOver);
	UNSUBSCRIBE_EVENT(OverrideUIName);
	UNSUBSCRIBE_EVENT(OnSOActionAvailabile);

	Actions.Clear();
	Game::CProperty::Deactivate();
}
//---------------------------------------------------------------------

bool CPropUIControl::OnPropsActivated(const CEventBase& Event)
{
	CPropSmartObject* pSO = GetEntity()->FindProperty<CPropSmartObject>();
	if (!pSO) OK;

	const CPropSmartObject::CActList& SOActions = pSO->GetActions();

	for (int i = 0; i < SOActions.Size(); ++i)
	{
		CStrID ID = SOActions.KeyAtIndex(i);
		PSmartObjAction Act = SOActions.ValueAtIndex(i);
		if (Act.isvalid() && Act->AppearsInUI)
		{
			LPCSTR pUIName = SOActionNames.isvalid() ? SOActionNames->Get<nString>(ID, nString::Empty).Get() : NULL;
			n_assert(AddActionHandler(ID, pUIName, this, &CPropUIControl::OnExecuteSmartObjAction, DEFAULT_PRIORITY, true));

			CAction* pAction = GetActionByID(ID);
			n_assert(pAction);
			pAction->Visible = Act->Enabled;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool CPropUIControl::OnObjMouseOver(const Events::CEventBase& Event)
{
	if ((*((Events::CEvent&)Event).Params).Get<bool>(CStrID("IsOver"), false))
	{
		PParams P = n_new(CParams);
		P->Set(CStrID("Text"), UIName.IsValid() ? UIName : nString(GetEntity()->GetUniqueID().CStr()));
		P->Set(CStrID("EntityID"), GetEntity()->GetUniqueID());
		EventMgr->FireEvent(CStrID("ShowIAOTip"), P);
	}
	else EventMgr->FireEvent(CStrID("HideIAOTip")); //!!!later should send entity ID here to identify which tip to hide!

	OK;
}
//---------------------------------------------------------------------

bool CPropUIControl::OverrideUIName(const Events::CEventBase& Event)
{
	UIName = ((Events::CEvent&)Event).Params->Get<nString>(CStrID("Text"));
	OK;
}
//---------------------------------------------------------------------

bool CPropUIControl::OnSOActionAvailabile(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	CAction* pAction = GetActionByID(P->Get<CStrID>(CStrID("ActionID")));
	if (pAction) pAction->Visible = P->Get<bool>(CStrID("Enabled"));
	OK;
}
//---------------------------------------------------------------------

bool CPropUIControl::AddActionHandler(CStrID ID, LPCSTR UIName, LPCSTR ScriptFuncName, int Priority, bool AutoAdded)
{
	CPropScriptable* pScriptable = GetEntity()->FindProperty<CPropScriptable>();
	CScriptObject* pScriptObj = pScriptable ? pScriptable->GetScriptObject() : NULL;
	if (!pScriptObj) FAIL;
	return AddActionHandler(ID, UIName, n_new(CEventHandlerScript)(pScriptObj, ScriptFuncName), Priority, AutoAdded);
}
//---------------------------------------------------------------------

bool CPropUIControl::AddActionHandler(CStrID ID, LPCSTR UIName, Events::PEventHandler Handler, int Priority, bool AutoAdded)
{
	for (nArray<CAction>::iterator It = Actions.Begin(); It != Actions.End(); It++)
		if (It->ID == ID) FAIL;

	CAction Act(ID, UIName, Priority);

	char EvIDString[64];
	sprintf_s(EvIDString, 63, "OnUIAction%s", ID.CStr());
	Act.EventID = CStrID(EvIDString);
	Act.Sub = GetEntity()->AddHandler(Act.EventID, Handler);
	if (!Act.Sub.isvalid()) FAIL;
	Act.AutoAdded = AutoAdded;

	Actions.InsertSorted(Act);
	
	OK;
}
//---------------------------------------------------------------------

void CPropUIControl::RemoveActionHandler(CStrID ID)
{
	for (nArray<CAction>::iterator It = Actions.Begin(); It != Actions.End(); It++)
		if (It->ID == ID)
		{
			Actions.Erase(It);
			return;
		}
}
//---------------------------------------------------------------------

bool CPropUIControl::ExecuteAction(Game::CEntity* pActorEnt, CAction& Action)
{
	if (!Action.Enabled) FAIL;

	PParams P = n_new(CParams);
	P->Set(CStrID("ActorEntityPtr"), (PVOID)pActorEnt);
	P->Set(CStrID("ActionID"), Action.ID);
	GetEntity()->FireEvent(Action.EventID, P);

	OK;
}
//---------------------------------------------------------------------

bool CPropUIControl::ExecuteAction(Game::CEntity* pActorEnt, CStrID ID)
{
	CAction* pAction = GetActionByID(ID);
	if (!pAction) FAIL;
	if (pAction->AutoAdded)
	{
		CPropActorBrain* pActor = pActorEnt->FindProperty<CPropActorBrain>();
		CPropSmartObject* pSO = GetEntity()->FindProperty<CPropSmartObject>();
		n_assert(pActor && pSO);
		pAction->Enabled = pSO->GetAction(ID)->IsValid(pActor, pSO);
	}
	return pAction->Enabled && ExecuteAction(pActorEnt, *pAction);
}
//---------------------------------------------------------------------

bool CPropUIControl::ExecuteDefaultAction(Game::CEntity* pActorEnt)
{
	if (!pActorEnt || !Actions.Size()) FAIL;

	// Cmd can have the highest priority but be disabled. Imagine character under the 
	// silence spell who left-clicks on NPC. Default cmd is "Talk" which is disabled
	// and next cmd is "Attack". We definitely don't want to attack friendly NPC by
	// default (left-click) just because we can't speak at the moment xD

	CPropActorBrain* pActor = NULL;
	CPropSmartObject* pSO = NULL;
	if (AutoAddSmartObjActions)
	{
		pActor = pActorEnt->FindProperty<CPropActorBrain>();
		pSO = GetEntity()->FindProperty<CPropSmartObject>();
		n_assert(pActor && pSO);
	}

	CAction* pTopAction = Actions.Begin();
	for (nArray<CPropUIControl::CAction>::iterator It = Actions.Begin(); It != Actions.End(); It++)
	{
		if (It->AutoAdded)
		{
			It->Enabled = pSO->GetAction(It->ID)->IsValid(pActor, pSO);
			// Update Priority
		}
		if ((*It) < (*pTopAction)) pTopAction = It;
	}

	return ExecuteAction(pActorEnt, *pTopAction);
}
//---------------------------------------------------------------------

void CPropUIControl::ShowPopup(Game::CEntity* pActorEnt)
{
	CPropActorBrain* pActor = NULL;
	CPropSmartObject* pSO = NULL;
	if (AutoAddSmartObjActions)
	{
		pActor = pActorEnt->FindProperty<CPropActorBrain>();
		pSO = GetEntity()->FindProperty<CPropSmartObject>();
		n_assert(pActor && pSO);
	}

	int VisibleCount = 0;
	for (nArray<CPropUIControl::CAction>::iterator It = Actions.Begin(); It != Actions.End(); It++)
		if (It->Visible)
		{
			if (It->AutoAdded)
			{
				It->Enabled = pSO->GetAction(It->ID)->IsValid(pActor, pSO);
				// Update Priority
			}

			++VisibleCount;
		}

	if (!VisibleCount) return;

	Actions.Sort();

	// Don't pass ptrs if async!
	PParams P = n_new(CParams);
	P->Set(CStrID("ActorEntityPtr"), (PVOID)pActorEnt);
	P->Set(CStrID("CtlPtr"), (PVOID)this);
	EventMgr->FireEvent(CStrID("ShowActionListPopup"), P);
}
//---------------------------------------------------------------------

// Special handler for auto-added smart object actions
bool CPropUIControl::OnExecuteSmartObjAction(const Events::CEventBase& Event)
{
	CPropSmartObject* pSO = GetEntity()->FindProperty<CPropSmartObject>();
	n_assert(pSO);

	PParams P = ((const CEvent&)Event).Params;

	PTaskUseSmartObj Task = n_new(CTaskUseSmartObj);
	Task->SetSmartObj(pSO);
	Task->SetActionID(P->Get<CStrID>(CStrID("ActionID")));
	((Game::CEntity*)P->Get<PVOID>(CStrID("ActorEntityPtr")))->FireEvent(Event::QueueTask(Task));

	OK;
}
//---------------------------------------------------------------------

} // namespace Properties