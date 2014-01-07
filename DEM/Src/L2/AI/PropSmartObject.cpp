#include "PropSmartObject.h"

#include <Game/GameServer.h>
#include <AI/AIServer.h>
#include <Animation/PropAnimation.h>
#include <Scripting/PropScriptable.h>
#include <Events/EventServer.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>

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

	CPropScriptable* pPropScript = GetEntity()->GetProperty<CPropScriptable>();
	if (pPropScript && pPropScript->IsActive())
	{
		EnableSI(*pPropScript);
		GetEntity()->FireEvent(CStrID("OnSOLoaded")); //???or in OnPropsActivated?
	}

	CPropAnimation* pPropAnim = GetEntity()->GetProperty<CPropAnimation>();
	if (pPropAnim && pPropAnim->IsActive()) LoadAnimationInfo(Desc, *pPropAnim);

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
	ActionAnims.Clear();
	StateAnims.Clear();
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
	SetState(Desc->Get<CStrID>(CStrID("DefaultState")), CStrID::Empty, 0.f);
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

	if (pProp->IsA<CPropAnimation>())
	{
		const CString& DescResource = GetEntity()->GetAttr<CString>(CStrID("SODesc"), NULL);
		Data::PParams Desc = DataSrv->LoadPRM(CString("Smarts:") + DescResource + ".prm");
		LoadAnimationInfo(Desc, *(CPropAnimation*)pProp);
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

	if (pProp->IsA<CPropAnimation>())
	{
		ActionAnims.Clear();
		StateAnims.Clear();
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

bool CPropSmartObject::OnBeginFrame(const Events::CEventBase& Event)
{
	SetTransitionProgress(TrProgress + (float)GameSrv->GetFrameTime());
	OK;
}
//---------------------------------------------------------------------

void CPropSmartObject::LoadAnimationInfo(Data::PParams Desc, CPropAnimation& Prop)
{
	Data::PParams Anims;
	if (Desc->Get<Data::PParams>(Anims, CStrID("ActionAnims")))
	{
		for (int i = 0; i < Anims->GetCount(); ++i)
		{
			Data::CParam& Prm = Anims->Get(i);
			CAnimInfo& AnimInfo = ActionAnims.Add(Prm.GetName());
			FillAnimationInfo(AnimInfo, *Prm.GetValue<Data::PParams>(), Prop);
		}
	}

	if (Desc->Get<Data::PParams>(Anims, CStrID("StateAnims")))
	{
		for (int i = 0; i < Anims->GetCount(); ++i)
		{
			Data::CParam& Prm = Anims->Get(i);
			CAnimInfo& AnimInfo = StateAnims.Add(Prm.GetName());
			FillAnimationInfo(AnimInfo, *Prm.GetValue<Data::PParams>(), Prop);
		}
	}
}
//---------------------------------------------------------------------

//???priority, fadein, fadeout?
void CPropSmartObject::FillAnimationInfo(CAnimInfo& AnimInfo, const Data::CParams& Desc, class CPropAnimation& Prop)
{
	AnimInfo.ClipID = Desc.Get<CStrID>(CStrID("Clip"), CStrID::Empty);
	n_assert(AnimInfo.ClipID.IsValid());
	AnimInfo.Loop = Desc.Get(CStrID("Loop"), false);
	AnimInfo.Speed = Desc.Get(CStrID("Speed"), 1.f);
	AnimInfo.Weight = Desc.Get(CStrID("Weight"), 1.f);
	AnimInfo.Duration = Prop.GetAnimLength(AnimInfo.ClipID);
	if (Desc.Get(AnimInfo.Offset, CStrID("RelOffset")))
		AnimInfo.Offset *= AnimInfo.Duration;
	else AnimInfo.Offset = Desc.Get(CStrID("Offset"), 0.f);
}
//---------------------------------------------------------------------

bool CPropSmartObject::SetState(CStrID StateID, CStrID ActionID, float TransitionDuration, bool ManualControl)
{
	n_assert2(StateID.IsValid(), "CPropSmartObject::SetState() > Tried to set empty state");

	int Idx = ActionAnims.FindIndex(ActionID);
	CAnimInfo* pAnimInfo = (Idx != INVALID_INDEX) ? &ActionAnims.ValueAt(Idx) : NULL;
	if (TransitionDuration < 0.f)
	{
		if (pAnimInfo) TransitionDuration = pAnimInfo->Duration; //???Speed?
		else TransitionDuration = 0.f;
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

	TargetState = StateID;

	//???what if AbortTransition above was called?
	if (TransitionDuration == 0.f)
	{
		CompleteTransition();
		OK;
	}

	TrActionID = ActionID;
	TrManualControl = ManualControl;
	TrDuration = TransitionDuration;

	Data::PParams P = n_new(Data::CParams(2));
	P->Set(CStrID("From"), CurrState);
	P->Set(CStrID("To"), TargetState);
	if (CurrState.IsValid()) GetEntity()->FireEvent(CStrID("OnSOStateLeave"), P);

	SwitchAnimation(pAnimInfo);

	if (!ManualControl) SUBSCRIBE_PEVENT(OnBeginFrame, CPropSmartObject, OnBeginFrame);

	OK;
}
//---------------------------------------------------------------------

void CPropSmartObject::CompleteTransition()
{
	UNSUBSCRIBE_EVENT(OnBeginFrame);

	int Idx = StateAnims.FindIndex(TargetState);
	SwitchAnimation((Idx != INVALID_INDEX) ? &StateAnims.ValueAt(Idx) : NULL);

	Data::PParams P = n_new(Data::CParams(2));
	P->Set(CStrID("From"), CurrState);
	P->Set(CStrID("To"), TargetState);

	CurrState = TargetState;
	GetEntity()->SetAttr(CStrID("SOState"), CurrState);

	GetEntity()->FireEvent(CStrID("OnSOStateEnter"), P);
}
//---------------------------------------------------------------------

void CPropSmartObject::SetTransitionDuration(float Time)
{
	//???update progress and animation?

	if (!IsInTransition() || Time < 0.f) return;
	TrDuration = Time;
	if (TrProgress >= TrDuration) CompleteTransition();
}
//---------------------------------------------------------------------

void CPropSmartObject::SetTransitionProgress(float Time)
{
	if (!IsInTransition()) return;

	TrProgress = Clamp(Time, 0.f, TrDuration);

	if (AnimTaskID != INVALID_INDEX)
	{
		CPropAnimation* pPropAnim = GetEntity()->GetProperty<CPropAnimation>();
		if (pPropAnim)
		{
			float CursorPos = pCurrAnimInfo->Offset + TrProgress * pCurrAnimInfo->Speed;
			if (!pCurrAnimInfo->Loop && pCurrAnimInfo->Duration != TrProgress && TrDuration != 0.f)
				CursorPos *= pCurrAnimInfo->Duration / TrDuration;
			pPropAnim->SetAnimCursorPos(AnimTaskID, CursorPos);
		}
	}

	if (TrProgress >= TrDuration) CompleteTransition();
}
//---------------------------------------------------------------------

void CPropSmartObject::AbortTransition(float Duration)
{
	//!!!implement nonzero duration!

	//???interchange curr and target and complete transition?

	int Idx = StateAnims.FindIndex(CurrState);
	SwitchAnimation((Idx != INVALID_INDEX) ? &StateAnims.ValueAt(Idx) : NULL);

	TargetState = CurrState;
	TrProgress = 0.f;
	TrDuration = 0.f;
	TrActionID = CStrID::Empty;

	// Notify about state re-entering
	Data::PParams P = n_new(Data::CParams(2));
	P->Set(CStrID("From"), CurrState);
	P->Set(CStrID("To"), CurrState);
	GetEntity()->FireEvent(CStrID("OnSOStateEnter"), P);
}
//---------------------------------------------------------------------

void CPropSmartObject::SwitchAnimation(const CAnimInfo* pAnimInfo)
{
	if (AnimTaskID == INVALID_INDEX && !pAnimInfo) return;
	CPropAnimation* pPropAnim = GetEntity()->GetProperty<CPropAnimation>();
	if (!pPropAnim) return;

	//???!!!allow to update the last frame?!
	if (AnimTaskID != INVALID_INDEX) pPropAnim->StopAnim(AnimTaskID);

	AnimTaskID = pAnimInfo ?
		pPropAnim->StartAnim(pAnimInfo->ClipID, pAnimInfo->Loop, pAnimInfo->Offset, pAnimInfo->Speed, true, AnimPriority_Default, pAnimInfo->Weight) :
		INVALID_INDEX;
	pCurrAnimInfo = pAnimInfo;
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