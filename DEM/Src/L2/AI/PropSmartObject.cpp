#include "PropSmartObject.h"

#include <Game/Entity.h>
#include <Scripting/PropScriptable.h>
#include <AI/AIServer.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <Events/EventServer.h>

namespace Prop
{
__ImplementClass(Prop::CPropSmartObject, 'PRSO', Game::CProperty);
__ImplementPropertyStorage(CPropSmartObject);

bool CPropSmartObject::InternalActivate()
{
	CurrState = GetEntity()->GetAttr(CStrID("SOState"), CStrID::Empty);
	GetEntity()->SetAttr(CStrID("SOState"), CurrState); // Make sure the attribute is set

	Data::PParams Desc;
	const CString& DescResource = GetEntity()->GetAttr<CString>(CStrID("SODesc"), NULL);
	if (DescResource.IsValid()) Desc = DataSrv->LoadPRM(CString("Smarts:") + DescResource + ".prm");

	if (Desc.IsValid())
	{
		TypeID = Desc->Get(CStrID("TypeID"), CStrID::Empty);
		Movable = Desc->Get(CStrID("Movable"), false);

		Data::PParams ActionsEnabled;
		GetEntity()->GetAttr(ActionsEnabled, CStrID("SOActionsEnabled"));

		Data::PParams DescSection;
		if (Desc->Get<Data::PParams>(DescSection, CStrID("Actions")))
		{
			Actions.BeginAdd(DescSection->GetCount());
			for (int i = 0; i < DescSection->GetCount(); i++)
			{
				const Data::CParam& Prm = DescSection->Get(i);
				const AI::CSmartAction* pTpl = AISrv->GetSmartAction(Prm.GetValue<CStrID>());
				if (pTpl)
				{
					CAction& Action = Actions.Add(Prm.GetName());
					Action.pTpl = pTpl;
					Action.FreeUserSlots = pTpl->MaxUserCount;
					if (CurrState.IsValid() && ActionsEnabled.IsValid()) ActionsEnabled->Get(Action.Enabled, Prm.GetName());
					else Action.Enabled = false;
				}
				else n_printf("AI, SO, Warning: can't find smart object action template '%s'\n", Prm.GetValue<CStrID>().CStr());
			}
			Actions.EndAdd();
		}
	}

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive())
	{
		EnableSI(*pProp);
		GetEntity()->FireEvent(CStrID("OnSOLoaded")); //???or in OnPropsActivated?
	}

	if (!CurrState.IsValid() && Desc.IsValid() && Desc->Has(CStrID("DefaultState")))
	{
		// Delayed default state setting, see comment to an OnPropsActivated()
		PROP_SUBSCRIBE_PEVENT(OnPropsActivated, CPropSmartObject, OnPropsActivated);
	}
	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropSmartObject, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnPropDeactivating, CPropSmartObject, OnPropDeactivating);
	PROP_SUBSCRIBE_PEVENT(OnLevelSaving, CPropSmartObject, OnLevelSaving);
	OK;
}
//---------------------------------------------------------------------

void CPropSmartObject::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(OnPropsActivated);
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnPropDeactivating);
	UNSUBSCRIBE_EVENT(OnLevelSaving);
	UNSUBSCRIBE_EVENT(OnBeginFrame);

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive()) DisableSI(*pProp);

	CurrState = CStrID::Empty;
	Actions.Clear();
}
//---------------------------------------------------------------------

// Empty state has a non-conditional transition to DefaultState from the Desc, if it is defined.
// Default state must be set through SetState, and script feedback is desired. We delay state
// setting to make sure that a possible PropScriptable is activated. If you want to use empty
// state as 'Disabled', create explicit 'Disabled' state without this hardcoded transition.
// NB: all checks were already performed on subscription.
bool CPropSmartObject::OnPropsActivated(const Events::CEventBase& Event)
{
	const CString& DescResource = GetEntity()->GetAttr<CString>(CStrID("SODesc"), NULL);
	Data::PParams Desc = DataSrv->LoadPRM(CString("Smarts:") + DescResource + ".prm");
	SetTransitionDuration(0.f);
	SetState(Desc->Get<CStrID>(CStrID("DefaultState")));
	OK;
}
//---------------------------------------------------------------------

bool CPropSmartObject::OnPropActivated(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropScriptable>())
	{
		EnableSI(*(CPropScriptable*)pProp);
		GetEntity()->FireEvent(CStrID("OnSOLoaded")); //???or in OnPropsActivated?
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropSmartObject::OnPropDeactivating(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropScriptable>())
	{
		DisableSI(*(CPropScriptable*)pProp);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropSmartObject::OnLevelSaving(const Events::CEventBase& Event)
{
	// Need to recreate params because else we may rewrite initial level desc in the HRD cache
	Data::PParams P = n_new(Data::CParams(Actions.GetCount()));
	for (int i = 0; i < Actions.GetCount(); ++i)
		P->Set(Actions.KeyAt(i), Actions.ValueAt(i).Enabled);
	GetEntity()->SetAttr<Data::PParams>(CStrID("SOActionsEnabled"), P);
	OK;
}
//---------------------------------------------------------------------

bool CPropSmartObject::SetState(CStrID StateID, CStrID ActionID, float TransitionDuration, bool ManualControl)
{
	n_assert2(StateID.IsValid(), "CPropSmartObject::SetState() > Tried to set empty state");

	CAction* pAction;
	if (ActionID.IsValid())
	{
		pAction = GetAction(ActionID);
		if (!pAction) FAIL;
	}
	else pAction = NULL;

	if (TransitionDuration < 0.f)
	{
		//get animation, get its duration and set as TransitionDuration
		//If no animation, TransitionDuration = 0.f;
	}

	if (CurrState == StateID)
	{
		if (IsInTransition())
		{
			//// We are in a transition from the current state and are requested
			//// to return to this state. Get current transition progress and remap
			//// it to the new duration to get return time.
			//float Time;
			//if (TrDuration == 0.f) Time = 0.f;
			//else
			//{
			//	Time = TrProgress;
			//	if (TransitionDuration != TrDuration)
			//		Time *= (TransitionDuration / TrDuration);
			//}
			AbortTransition(/*Time*/); //???manul or always automatic?
		}
		OK;
	}

	TargetState = StateID;

	if (TrDuration == 0.f)
	{
		CompleteTransition();
		OK;
			//Data::PParams P = n_new(Data::CParams(2));
			//P->Set(CStrID("From"), CurrState);
			//P->Set(CStrID("To"), TargetState);
			//GetEntity()->FireEvent(CStrID("OnSOStateEnter"), P);

			//CurrState = TargetState;
			//GetEntity()->SetAttr(CStrID("SOState"), CurrState);
	}

	if (IsInTransition())
	{
		if (TargetState == StateID)
		{
			// The same transition paused, resume it and remap progress to a new duration
			if (TrDuration == 0.f) TrProgress = 0.f;
			else if (TransitionDuration != TrDuration)
				TrProgress *= (TransitionDuration / TrDuration);
		}
		else
		{
			// Other transition, abort it immediately and start requested one
			AbortTransition();
			TrProgress = 0.f;
		}
	}
	else TrProgress = 0.f;

	TrActionID = ActionID;
	TrManualControl = ManualControl;
	TrDuration = TransitionDuration;

	Data::PParams P = n_new(Data::CParams(2));
	P->Set(CStrID("From"), CurrState);
	P->Set(CStrID("To"), TargetState);
	if (CurrState.IsValid()) GetEntity()->FireEvent(CStrID("OnSOStateLeave"), P);

	//init transition animation

	if (!ManualControl) SUBSCRIBE_PEVENT(OnBeginFrame, CPropSmartObject, OnBeginFrame);

	OK;
}
//---------------------------------------------------------------------

void CPropSmartObject::EnableAction(CStrID ID, bool Enable)
{
	int Idx = Actions.FindIndex(ID);
	if (Idx == INVALID_INDEX) return;

	CAction& Action = Actions.ValueAt(Idx);
	if (Action.Enabled == Enable) return;
	Action.Enabled = Enable;

	Data::PParams P = n_new(Data::CParams(2));
	P->Set(CStrID("ActionID"), ID);
	P->Set(CStrID("Enabled"), Enable);
	GetEntity()->FireEvent(CStrID("OnSOActionAvailabile"), P);
}
//---------------------------------------------------------------------

bool CPropSmartObject::GetDestinationParams(CStrID ActionID, float ActorRadius, vector3& OutOffset, float& OutMinDist, float& OutMaxDist)
{
	const CAction* pAction = GetAction(ActionID);

	if (pAction)
	{
		matrix33 Tfm = GetEntity()->GetAttr<matrix44>(CStrID("Transform")).ToMatrix33();
		Tfm.mult(pAction->pTpl->DestOffset, OutOffset);
		OutMinDist = pAction->pTpl->MinDistance;
		OutMaxDist = pAction->pTpl->MaxDistance;
		if (pAction->pTpl->ActorRadiusMatters())
		{
			OutMinDist += ActorRadius;
			OutMaxDist += ActorRadius;
		}
		//???add SORadiusMatters? for items, enemies etc
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

}