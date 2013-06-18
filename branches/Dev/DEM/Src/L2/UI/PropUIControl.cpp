#include "PropUIControl.h"

#include <Game/Entity.h>
#include <Game/GameLevel.h>
#include <AI/Events/QueueTask.h>
#include <AI/PropActorBrain.h>
#include <AI/PropSmartObject.h>
#include <AI/SmartObj/Tasks/TaskUseSmartObj.h>
#include <Physics/PhysicsServer.h>
#include <Scene/PropSceneNode.h>
#include <Scripting/PropScriptable.h>
#include <Scripting/EventHandlerScript.h>
#include <Data/DataServer.h>
#include <Events/EventManager.h>

namespace Physics
{
	PCollisionShape LoadCollisionShapeFromPRM(CStrID UID, const nString& FileName);
}

namespace Prop
{
__ImplementClass(Prop::CPropUIControl, 'PUIC', Game::CProperty);
__ImplementPropertyStorage(CPropUIControl);

using namespace Data;

bool CPropUIControl::InternalActivate()
{
	UIName = GetEntity()->GetAttr<nString>(CStrID("Name"), NULL);
	UIDesc = GetEntity()->GetAttr<nString>(CStrID("Desc"), NULL);
	ReflectSOActions = false;

	const nString& IAODesc = GetEntity()->GetAttr<nString>(CStrID("IAODesc"), NULL);
	PParams Desc = IAODesc.IsValid() ? DataSrv->LoadPRM(nString("iao:") + IAODesc + ".prm") : NULL;
	if (Desc.IsValid())
	{
		if (UIName.IsEmpty()) UIName = Desc->Get<nString>(CStrID("UIName"), NULL);
		if (UIDesc.IsEmpty()) UIDesc = Desc->Get<nString>(CStrID("UIDesc"), NULL);

		//???read priorities for actions? or all through scripts?

		Data::PParams UIActionNames = Desc->Get<PParams>(CStrID("UIActionNames"), NULL);

		if (Desc->Get<bool>(CStrID("Explorable"), false))
		{
			CStrID ID("Explore");
			LPCSTR pUIName = UIActionNames.IsValid() ? UIActionNames->Get<nString>(ID, nString::Empty).CStr() : ID.CStr();
			n_assert(AddActionHandler(ID, pUIName, this, &CPropUIControl::OnExecuteExploreAction, 1, false));
			CAction* pAct = GetActionByID(ID);
			pAct->Enabled = UIDesc.IsValid();
			pAct->Visible = pAct->Enabled;
		}

		if (Desc->Get<bool>(CStrID("Selectable"), false))
		{
			CStrID ID("Select");
			LPCSTR pUIName = UIActionNames.IsValid() ? UIActionNames->Get<nString>(ID, nString::Empty).CStr() : ID.CStr();
			n_assert(AddActionHandler(ID, pUIName, this, &CPropUIControl::OnExecuteSelectAction, Priority_Top, false));
		}

		EnableSmartObjReflection(Desc->Get<bool>(CStrID("AutoAddSmartObjActions"), true));
	}

	//???move to the IAO desc as field? per-entity allows not to spawn redundant IAO descs
	//???Shape desc or collision object desc? CollObj can have offset, but Group & Mask must be overridden to support picking.
	CStrID PickShapeID = GetEntity()->GetAttr<CStrID>(CStrID("PickShape"), CStrID::Empty);
	if (PickShapeID.IsValid() && GetEntity()->GetLevel().GetPhysics())
	{
		Physics::PCollisionShape Shape = PhysicsSrv->CollisionShapeMgr.GetTypedResource(PickShapeID);
		if (!Shape.IsValid())
			Shape = Physics::LoadCollisionShapeFromPRM(PickShapeID, nString("physics:") + PickShapeID.CStr() + ".hrd"); //!!!prm!
		n_assert(Shape->IsLoaded());

		ushort Group = PhysicsSrv->CollisionGroups.GetMask("MousePickTarget");
		ushort Mask = PhysicsSrv->CollisionGroups.GetMask("MousePick");

		//???or use OnUpdateTransform?
		MousePickShape = n_new(Physics::CNodeAttrCollision);
		MousePickShape->CollObj = n_new(Physics::CCollisionObjMoving);
		MousePickShape->CollObj->Init(*Shape, Group, Mask); // Can specify offset
		MousePickShape->CollObj->SetUserData(*(void**)&GetEntity()->GetUID());
		MousePickShape->CollObj->SetTransform(GetEntity()->GetAttr<matrix44>(CStrID("Transform")));

		CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
		if (pProp && pProp->IsActive())
		{
			MousePickShape->CollObj->AttachToLevel(*GetEntity()->GetLevel().GetPhysics());
			pProp->GetNode()->AddAttr(*MousePickShape);
		}
	}

	PROP_SUBSCRIBE_PEVENT(ExposeSI, CPropUIControl, ExposeSI);
	PROP_SUBSCRIBE_PEVENT(OnMouseEnter, CPropUIControl, OnMouseEnter);
	PROP_SUBSCRIBE_PEVENT(OnMouseLeave, CPropUIControl, OnMouseLeave);
	OK;
}
//---------------------------------------------------------------------

void CPropUIControl::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(ExposeSI);
	UNSUBSCRIBE_EVENT(OnMouseEnter);
	UNSUBSCRIBE_EVENT(OnMouseLeave);
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnPropDeactivating);
	UNSUBSCRIBE_EVENT(OnSOActionAvailabile);

	HideTip();

	Actions.Clear();

	if (MousePickShape.IsValid())
	{
		MousePickShape->RemoveFromNode();
		MousePickShape->CollObj->RemoveFromLevel();
		MousePickShape = NULL;
	}
}
//---------------------------------------------------------------------

bool CPropUIControl::OnPropActivated(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (ReflectSOActions && pProp->IsA<CPropSmartObject>())
	{
		AddSOActions(*(CPropSmartObject*)pProp);
		OK;
	}

	if (MousePickShape.IsValid() && pProp->IsA<CPropSceneNode>() && ((CPropSceneNode*)pProp)->GetNode())
	{
		MousePickShape->CollObj->AttachToLevel(*GetEntity()->GetLevel().GetPhysics());
		((CPropSceneNode*)pProp)->GetNode()->AddAttr(*MousePickShape);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropUIControl::OnPropDeactivating(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (ReflectSOActions && pProp->IsA<CPropSmartObject>())
	{
		RemoveSOActions();
		OK;
	}

	if (MousePickShape.IsValid() && pProp->IsA<CPropSceneNode>())
	{
		MousePickShape->RemoveFromNode();
		MousePickShape->CollObj->RemoveFromLevel();
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

void CPropUIControl::AddSOActions(CPropSmartObject& Prop)
{
	const CPropSmartObject::CActList& SOActions = Prop.GetActions();

	const nString& IAODesc = GetEntity()->GetAttr<nString>(CStrID("IAODesc"), NULL);
	PParams Desc = IAODesc.IsValid() ? DataSrv->LoadPRM(nString("iao:") + IAODesc + ".prm") : NULL;
	Data::PParams SOActionNames = Desc.IsValid() ? Desc->Get<PParams>(CStrID("SmartObjActionNames"), NULL) : NULL;

	for (int i = 0; i < SOActions.GetCount(); ++i)
	{
		CStrID ID = SOActions.KeyAt(i);
		PSmartObjAction Act = SOActions.ValueAt(i);
		if (Act.IsValid() && Act->AppearsInUI)
		{
			LPCSTR pUIName = SOActionNames.IsValid() ? SOActionNames->Get<nString>(ID, nString::Empty).CStr() : NULL;
			n_assert(AddActionHandler(ID, pUIName, this, &CPropUIControl::OnExecuteSmartObjAction, Priority_Default, true));

			CAction* pAction = GetActionByID(ID);
			n_assert(pAction);
			pAction->Visible = Act->Enabled;
		}
	}
}
//---------------------------------------------------------------------

void CPropUIControl::RemoveSOActions()
{
	for (int i = 0 ; i < Actions.GetCount(); )
	{
		if (Actions[i].IsSOAction) Actions.EraseAt(i);
		else ++i;
	}
}
//---------------------------------------------------------------------

void CPropUIControl::EnableSmartObjReflection(bool Enable)
{
	if (ReflectSOActions == Enable) return;
	ReflectSOActions = Enable;
	if (ReflectSOActions)
	{
		CPropSmartObject* pProp = GetEntity()->GetProperty<CPropSmartObject>();
		if (pProp && pProp->IsActive()) AddSOActions(*(CPropSmartObject*)pProp);

		PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropUIControl, OnPropActivated);
		PROP_SUBSCRIBE_PEVENT(OnPropDeactivating, CPropUIControl, OnPropDeactivating);
		PROP_SUBSCRIBE_PEVENT(OnSOActionAvailabile, CPropUIControl, OnSOActionAvailabile);
	}
	else
	{
		UNSUBSCRIBE_EVENT(OnPropActivated);
		UNSUBSCRIBE_EVENT(OnPropDeactivating);
		UNSUBSCRIBE_EVENT(OnSOActionAvailabile);

		RemoveSOActions();
	}
}
//---------------------------------------------------------------------

bool CPropUIControl::OnMouseEnter(const Events::CEventBase& Event)
{
	ShowTip();
	OK;
}
//---------------------------------------------------------------------

bool CPropUIControl::OnMouseLeave(const Events::CEventBase& Event)
{
	HideTip();
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

void CPropUIControl::Enable(bool SetEnabled)
{
	if (Enabled == SetEnabled) return;

	if (!SetEnabled)
	{
		Data::PParams P = n_new(Data::CParams(1));
		P->Set<PVOID>(CStrID("CtlPtr"), this);
		EventMgr->FireEvent(CStrID("HideActionListPopup"), P);
	}

	Enabled = SetEnabled;
}
//---------------------------------------------------------------------

void CPropUIControl::SetUIName(const nString& NewName)
{
	//???use attribute?
	UIName = NewName;
	if (TipVisible) ShowTip();
}
//---------------------------------------------------------------------

void CPropUIControl::ShowTip()
{
	PParams P = n_new(CParams);
	P->Set(CStrID("Text"), UIName.IsValid() ? UIName : nString(GetEntity()->GetUID().CStr()));
	P->Set(CStrID("EntityID"), GetEntity()->GetUID());
	TipVisible = (EventMgr->FireEvent(CStrID("ShowIAOTip"), P) > 0);
}
//---------------------------------------------------------------------

void CPropUIControl::HideTip()
{
	if (!TipVisible) return;
	EventMgr->FireEvent(CStrID("HideIAOTip")); //!!!later should send entity ID here to identify which tip to hide!
	TipVisible = false;
}
//---------------------------------------------------------------------

bool CPropUIControl::AddActionHandler(CStrID ID, LPCSTR UIName, LPCSTR ScriptFuncName, int Priority, bool IsSOAction)
{
	CPropScriptable* pScriptable = GetEntity()->GetProperty<CPropScriptable>();
	CScriptObject* pScriptObj = pScriptable ? pScriptable->GetScriptObject() : NULL;
	if (!pScriptObj) FAIL;
	return AddActionHandler(ID, UIName, n_new(Events::CEventHandlerScript)(pScriptObj, ScriptFuncName), Priority, IsSOAction);
}
//---------------------------------------------------------------------

bool CPropUIControl::AddActionHandler(CStrID ID, LPCSTR UIName, Events::PEventHandler Handler, int Priority, bool IsSOAction)
{
	for (nArray<CAction>::CIterator It = Actions.Begin(); It != Actions.End(); It++)
		if (It->ID == ID) FAIL;

	CAction Act(ID, UIName, Priority);

	char EvIDString[64];
	sprintf_s(EvIDString, 63, "OnUIAction%s", ID.CStr());
	Act.EventID = CStrID(EvIDString);
	Act.Sub = GetEntity()->AddHandler(Act.EventID, Handler);
	if (!Act.Sub.IsValid()) FAIL;
	Act.IsSOAction = IsSOAction;

	Actions.InsertSorted(Act);
	
	OK;
}
//---------------------------------------------------------------------

void CPropUIControl::RemoveActionHandler(CStrID ID)
{
	for (nArray<CAction>::CIterator It = Actions.Begin(); It != Actions.End(); It++)
		if (It->ID == ID)
		{
			Actions.Erase(It);
			return;
		}
}
//---------------------------------------------------------------------

bool CPropUIControl::ExecuteAction(Game::CEntity* pActorEnt, CAction& Action)
{
	if (!Enabled || !Action.Enabled) FAIL;

	PParams P = n_new(CParams);
	P->Set(CStrID("ActorEntityPtr"), (PVOID)pActorEnt);
	P->Set(CStrID("ActionID"), Action.ID);
	GetEntity()->FireEvent(Action.EventID, P);

	OK;
}
//---------------------------------------------------------------------

bool CPropUIControl::ExecuteAction(Game::CEntity* pActorEnt, CStrID ID)
{
	if (!Enabled) FAIL;

	CAction* pAction = GetActionByID(ID);
	if (!pAction) FAIL;
	if (pAction->IsSOAction)
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
	if (!Enabled || !pActorEnt || !Actions.GetCount()) FAIL;

	// Cmd can have the highest priority but be disabled. Imagine character under the 
	// silence spell who left-clicks on NPC. Default cmd is "Talk" which is disabled
	// and next cmd is "Attack". We definitely don't want to attack friendly NPC by
	// default (left-click) just because we can't speak at the moment xD

	CPropActorBrain* pActor = NULL;
	CPropSmartObject* pSO = NULL;
	if (ReflectSOActions)
	{
		pActor = pActorEnt->GetProperty<CPropActorBrain>();
		pSO = GetEntity()->GetProperty<CPropSmartObject>();
		n_assert(pActor && pSO);
	}

	CAction* pTopAction = Actions.Begin();
	for (nArray<CPropUIControl::CAction>::CIterator It = Actions.Begin(); It != Actions.End(); It++)
	{
		if (It->IsSOAction)
		{
			It->Enabled = pSO->GetAction(It->ID)->IsValid(pActor, pSO);
			// Update Priority
		}

		// FIXME
		// This line discards disbled actions, so for example door in transition between
		// Opened and Closed has default action Explore, which is executed on left click.
		// Need to solve this and choose desired behaviour. Force set default action?
		if ((*It) < (*pTopAction)) pTopAction = It;
	}

	return ExecuteAction(pActorEnt, *pTopAction);
}
//---------------------------------------------------------------------

void CPropUIControl::ShowPopup(Game::CEntity* pActorEnt)
{
	if (!Enabled) return;

	CPropActorBrain* pActor = NULL;
	CPropSmartObject* pSO = NULL;
	if (ReflectSOActions)
	{
		pActor = pActorEnt->GetProperty<CPropActorBrain>();
		pSO = GetEntity()->GetProperty<CPropSmartObject>();
		n_assert(pActor && pSO);
	}

	int VisibleCount = 0;
	for (nArray<CPropUIControl::CAction>::CIterator It = Actions.Begin(); It != Actions.End(); It++)
		if (It->Visible)
		{
			if (It->IsSOAction)
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

bool CPropUIControl::OnExecuteExploreAction(const Events::CEventBase& Event)
{
	if (!UIDesc.IsValid()) FAIL;
	PParams P = n_new(Data::CParams(1));
	P->Set<nString>(CStrID("UIDesc"), UIDesc);
	EventMgr->FireEvent(CStrID("OnObjectDescRequested"), P, EV_ASYNC);
	OK;
}
//---------------------------------------------------------------------

bool CPropUIControl::OnExecuteSelectAction(const Events::CEventBase& Event)
{
	GetEntity()->GetLevel().AddToSelection(GetEntity()->GetUID());
	OK;
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

}