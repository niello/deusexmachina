#include "PropUIControl.h"

#include <Game/Entity.h>
#include <AI/Events/QueueTask.h>
#include <AI/Prop/PropActorBrain.h>
#include <AI/Prop/PropSmartObject.h>
#include <AI/SmartObj/Tasks/TaskUseSmartObj.h>
#include <Scripting/PropScriptable.h>
#include <Scripting/EventHandlerScript.h>
#include <Data/DataServer.h>
#include <Events/EventManager.h>

//BEGIN_ATTRS_REGISTRATION(PropUIControl)
//	RegisterString(IAODesc, ReadOnly);
//	RegisterString(Name, ReadOnly);
//END_ATTRS_REGISTRATION

namespace Prop
{
__ImplementClass(Prop::CPropUIControl, 'PUIC', Game::CProperty);
__ImplementPropertyStorage(CPropUIControl);

using namespace Data;

void CPropUIControl::Activate()
{
	Game::CProperty::Activate();

	UIName = GetEntity()->GetAttr<nString>(CStrID("Name"), NULL);

	PParams Desc;
	const nString& IAODesc = GetEntity()->GetAttr<nString>(CStrID("IAODesc"), NULL);
	if (IAODesc.IsValid()) Desc = DataSrv->LoadPRM(nString("iao:") + IAODesc + ".prm");

	if (Desc.IsValid())
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
	PROP_SUBSCRIBE_PEVENT(OnMouseEnter, CPropUIControl, OnMouseEnter);
	PROP_SUBSCRIBE_PEVENT(OnMouseLeave, CPropUIControl, OnMouseLeave);
	PROP_SUBSCRIBE_PEVENT(OverrideUIName, CPropUIControl, OverrideUIName);
}
//---------------------------------------------------------------------

void CPropUIControl::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnPropsActivated);
	UNSUBSCRIBE_EVENT(ExposeSI);
	UNSUBSCRIBE_EVENT(OnMouseEnter);
	UNSUBSCRIBE_EVENT(OnMouseLeave);
	UNSUBSCRIBE_EVENT(OverrideUIName);
	UNSUBSCRIBE_EVENT(OnSOActionAvailabile);

	Actions.Clear();
	Game::CProperty::Deactivate();
}
//---------------------------------------------------------------------

bool CPropUIControl::OnPropsActivated(const Events::CEventBase& Event)
{
	CPropSmartObject* pSO = GetEntity()->GetProperty<CPropSmartObject>();
	if (!pSO) OK;

	const CPropSmartObject::CActList& SOActions = pSO->GetActions();

	for (int i = 0; i < SOActions.GetCount(); ++i)
	{
		CStrID ID = SOActions.KeyAtIndex(i);
		PSmartObjAction Act = SOActions.ValueAtIndex(i);
		if (Act.IsValid() && Act->AppearsInUI)
		{
			LPCSTR pUIName = SOActionNames.IsValid() ? SOActionNames->Get<nString>(ID, nString::Empty).CStr() : NULL;
			n_assert(AddActionHandler(ID, pUIName, this, &CPropUIControl::OnExecuteSmartObjAction, DEFAULT_PRIORITY, true));

			CAction* pAction = GetActionByID(ID);
			n_assert(pAction);
			pAction->Visible = Act->Enabled;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool CPropUIControl::OnMouseEnter(const Events::CEventBase& Event)
{
	PParams P = n_new(CParams);
	P->Set(CStrID("Text"), UIName.IsValid() ? UIName : nString(GetEntity()->GetUID().CStr()));
	P->Set(CStrID("EntityID"), GetEntity()->GetUID());
	EventMgr->FireEvent(CStrID("ShowIAOTip"), P);
	OK;
}
//---------------------------------------------------------------------

bool CPropUIControl::OnMouseLeave(const Events::CEventBase& Event)
{
	EventMgr->FireEvent(CStrID("HideIAOTip")); //!!!later should send entity ID here to identify which tip to hide!
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
	CPropScriptable* pScriptable = GetEntity()->GetProperty<CPropScriptable>();
	CScriptObject* pScriptObj = pScriptable ? pScriptable->GetScriptObject() : NULL;
	if (!pScriptObj) FAIL;
	return AddActionHandler(ID, UIName, n_new(Events::CEventHandlerScript)(pScriptObj, ScriptFuncName), Priority, AutoAdded);
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
	if (!Act.Sub.IsValid()) FAIL;
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
		CPropActorBrain* pActor = pActorEnt->GetProperty<CPropActorBrain>();
		CPropSmartObject* pSO = GetEntity()->GetProperty<CPropSmartObject>();
		n_assert(pActor && pSO);
		pAction->Enabled = pSO->GetAction(ID)->IsValid(pActor, pSO);
	}
	return pAction->Enabled && ExecuteAction(pActorEnt, *pAction);
}
//---------------------------------------------------------------------

bool CPropUIControl::ExecuteDefaultAction(Game::CEntity* pActorEnt)
{
	if (!pActorEnt || !Actions.GetCount()) FAIL;

	// Cmd can have the highest priority but be disabled. Imagine character under the 
	// silence spell who left-clicks on NPC. Default cmd is "Talk" which is disabled
	// and next cmd is "Attack". We definitely don't want to attack friendly NPC by
	// default (left-click) just because we can't speak at the moment xD

	CPropActorBrain* pActor = NULL;
	CPropSmartObject* pSO = NULL;
	if (AutoAddSmartObjActions)
	{
		pActor = pActorEnt->GetProperty<CPropActorBrain>();
		pSO = GetEntity()->GetProperty<CPropSmartObject>();
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
		pActor = pActorEnt->GetProperty<CPropActorBrain>();
		pSO = GetEntity()->GetProperty<CPropSmartObject>();
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
	CPropSmartObject* pSO = GetEntity()->GetProperty<CPropSmartObject>();
	n_assert(pSO);

	PParams P = ((const Events::CEvent&)Event).Params;

	PTaskUseSmartObj Task = n_new(CTaskUseSmartObj);
	Task->SetSmartObj(pSO);
	Task->SetActionID(P->Get<CStrID>(CStrID("ActionID")));
	((Game::CEntity*)P->Get<PVOID>(CStrID("ActorEntityPtr")))->FireEvent(Event::QueueTask(Task));

	OK;
}
//---------------------------------------------------------------------

} // namespace Prop