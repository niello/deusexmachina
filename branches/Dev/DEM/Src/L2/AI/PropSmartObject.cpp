#include "PropSmartObject.h"

#include <Game/GameServer.h>
#include <AI/AIServer.h>
#include <AI/PropActorBrain.h> // For GetEntity() only
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
					Action.Enabled = ActionsEnabled.IsValid() ? ActionsEnabled->Get(Action.Enabled, Prm.GetName()) : false;
				}
				else n_printf("AI, SO, Warning: can't find smart object action template '%s'\n", Prm.GetValue<CStrID>().CStr());
			}
			Actions.EndAdd();
		}
	}

	//AnimTaskID = INVALID_INDEX;
	//pCurrAnimInfo = NULL;

	CurrState = GetEntity()->GetAttr(CStrID("SOState"), CStrID::Empty);
	TargetState = GetEntity()->GetAttr(CStrID("SOTargetState"), CurrState);

	if (IsInTransition())
	{
		TrProgress = GetEntity()->GetAttr(CStrID("SOTrProgress"), 0.f);
		TrDuration = GetEntity()->GetAttr(CStrID("SOTrDuration"), 0.f);
		TrActionID = GetEntity()->GetAttr(CStrID("SOTrActionID"), CStrID::Empty);
		TrManualControl = GetEntity()->GetAttr(CStrID("SOTrManualControl"), false);

		// Resume saved transition
		PROP_SUBSCRIBE_PEVENT(OnPropsActivated, CPropSmartObject, OnPropsActivated);
	}
	else
	{
		TrProgress = 0.f;
		TrDuration = 0.f;
		TrActionID = CStrID::Empty;
		TrManualControl = false;

		// Make transition from NULL state to DefaultState
		if (!TargetState.IsValid() && Desc.IsValid() && Desc->Has(CStrID("DefaultState")))
			PROP_SUBSCRIBE_PEVENT(OnPropsActivated, CPropSmartObject, OnPropsActivated);
	}

	// Make sure the attribute is set
	GetEntity()->SetAttr(CStrID("SOState"), CurrState);

	CPropScriptable* pPropScript = GetEntity()->GetProperty<CPropScriptable>();
	if (pPropScript && pPropScript->IsActive()) EnableSI(*pPropScript);

	CPropAnimation* pPropAnim = GetEntity()->GetProperty<CPropAnimation>();
	if (pPropAnim && pPropAnim->IsActive()) InitAnimation(Desc, *pPropAnim);

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

	SwitchAnimation(NULL);

	//???delete related entity attrs?

	CurrState = CStrID::Empty;
	ActionAnims.Clear();
	StateAnims.Clear();
	Actions.Clear();
}
//---------------------------------------------------------------------

bool CPropSmartObject::OnPropsActivated(const Events::CEventBase& Event)
{
	// Initialize current state and transition.
	// Do it here to make sure that script is loaded and will process transition events.
	if (TargetState.IsValid())
		SetState(TargetState, TrActionID, TrDuration, TrManualControl);
	else
	{
		const CString& DescResource = GetEntity()->GetAttr<CString>(CStrID("SODesc"), NULL);
		Data::PParams Desc = DataSrv->LoadPRM(CString("Smarts:") + DescResource + ".prm");
		SetState(Desc->Get<CStrID>(CStrID("DefaultState"), CStrID::Empty), TrActionID, -1.f, TrManualControl);
	}
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
		OK;
	}

	if (pProp->IsA<CPropAnimation>())
	{
		const CString& DescResource = GetEntity()->GetAttr<CString>(CStrID("SODesc"), NULL);
		Data::PParams Desc = DataSrv->LoadPRM(CString("Smarts:") + DescResource + ".prm");
		InitAnimation(Desc, *(CPropAnimation*)pProp);
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
		AnimTaskID = INVALID_INDEX;
		pCurrAnimInfo = NULL;
		ActionAnims.Clear();
		StateAnims.Clear();
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropSmartObject::OnLevelSaving(const Events::CEventBase& Event)
{
	if (IsInTransition())
	{
		GetEntity()->SetAttr(CStrID("SOTargetState"), TargetState);
		GetEntity()->SetAttr(CStrID("SOTrProgress"), TrProgress);
		GetEntity()->SetAttr(CStrID("SOTrDuration"), TrDuration);
		GetEntity()->SetAttr(CStrID("SOTrActionID"), TrActionID);
		GetEntity()->SetAttr(CStrID("SOTrManualControl"), TrManualControl);
	}
	else
	{
		GetEntity()->DeleteAttr(CStrID("SOTargetState"));
		GetEntity()->DeleteAttr(CStrID("SOTrProgress"));
		GetEntity()->DeleteAttr(CStrID("SOTrDuration"));
		GetEntity()->DeleteAttr(CStrID("SOTrActionID"));
		GetEntity()->DeleteAttr(CStrID("SOTrManualControl"));
	}

	// Need to recreate params because else we may rewrite initial level desc in the HRD cache
	Data::PParams P = n_new(Data::CParams(Actions.GetCount()));
	for (int i = 0; i < Actions.GetCount(); ++i)
		P->Set(Actions.KeyAt(i), Actions.ValueAt(i).Enabled);
	GetEntity()->SetAttr(CStrID("SOActionsEnabled"), P);
	OK;
}
//---------------------------------------------------------------------

bool CPropSmartObject::OnBeginFrame(const Events::CEventBase& Event)
{
	float Time = (float)GameSrv->GetFrameTime();
	if (Time != 0.f) SetTransitionProgress(TrProgress + Time);
	OK;
}
//---------------------------------------------------------------------

void CPropSmartObject::InitAnimation(Data::PParams Desc, CPropAnimation& Prop)
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

	if (IsInTransition())
	{
		int Idx = ActionAnims.FindIndex(CurrState);
		SwitchAnimation((Idx != INVALID_INDEX) ? &ActionAnims.ValueAt(Idx) : NULL);
		UpdateAnimationCursor();
	}
	else
	{
		int Idx = StateAnims.FindIndex(CurrState);
		SwitchAnimation((Idx != INVALID_INDEX) ? &StateAnims.ValueAt(Idx) : NULL);
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

	if (!IsInTransition() && StateID == CurrState) OK;

	if (IsInTransition() && StateID != TargetState)
	{
		//if StateID == CurrState and bidirectional allowed
		//	(x->y)->x case
		//	invert params and launch normal transition (later in a regular way?)
		//	invert progress here, remap to a new duration later
		//	andmake params here looking as (x->y)->y
		//else
		AbortTransition();

		//!!!if not bidirectional, exit (x->y)->x here because it becomes (x->x)->x!
	}

	int Idx = ActionAnims.FindIndex(ActionID);
	CAnimInfo* pAnimInfo = (Idx != INVALID_INDEX) ? &ActionAnims.ValueAt(Idx) : NULL;
	if (TransitionDuration < 0.f)
		TransitionDuration = (pAnimInfo && pAnimInfo->Speed != 0.f) ? pAnimInfo->Duration / n_fabs(pAnimInfo->Speed) : 0.f;

	if (TargetState == StateID)
	{
		// The same transition, remap progress to a new duration
		if (TrDuration == 0.f) TrProgress = 0.f;
		else if (TransitionDuration != TrDuration)
			TrProgress *= (TransitionDuration / TrDuration);
	}
	else TrProgress = 0.f;

	if (!IsInTransition())
	{
		Data::PParams P = n_new(Data::CParams(2));
		P->Set(CStrID("From"), CurrState);
		P->Set(CStrID("To"), StateID);
		GetEntity()->FireEvent(CStrID("OnSOStateLeave"), P);
	}

	TargetState = StateID;

	if (TransitionDuration == 0.f)
	{
		CompleteTransition();
		OK;
	}

	TrActionID = ActionID;
	TrManualControl = ManualControl;
	TrDuration = TransitionDuration;

	SwitchAnimation(pAnimInfo);
	UpdateAnimationCursor();

	if (!ManualControl && !IS_SUBSCRIBED(OnBeginFrame))
		SUBSCRIBE_PEVENT(OnBeginFrame, CPropSmartObject, OnBeginFrame);

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
	if (!IsInTransition() || Time < 0.f) return;
	TrDuration = Time;
	if (TrProgress >= TrDuration) CompleteTransition();
}
//---------------------------------------------------------------------

void CPropSmartObject::SetTransitionProgress(float Time)
{
	if (!IsInTransition()) return;
	TrProgress = Clamp(Time, 0.f, TrDuration);
	UpdateAnimationCursor();
	if (TrProgress >= TrDuration) CompleteTransition();
}
//---------------------------------------------------------------------

void CPropSmartObject::AbortTransition(float Duration)
{
	//!!!implement nonzero duration!
	n_assert2(Duration == 0.f, "Implement nonzero duration!!!");

	//???interchange curr and target and complete transition?
	//???only if bidirectional transition available for this state pair?

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
	if (pAnimInfo == pCurrAnimInfo) return;
	if (AnimTaskID == INVALID_INDEX && !pAnimInfo) return;
	CPropAnimation* pPropAnim = GetEntity()->GetProperty<CPropAnimation>();
	if (!pPropAnim) return;

	if (AnimTaskID != INVALID_INDEX) pPropAnim->StopAnim(AnimTaskID);

	if (pAnimInfo)
	{
		if (pAnimInfo->Speed == 0.f)
		{
			pPropAnim->SetPose(pAnimInfo->ClipID, pAnimInfo->Offset, pAnimInfo->Loop);
			AnimTaskID = INVALID_INDEX;
		}
		else AnimTaskID = pPropAnim->StartAnim(pAnimInfo->ClipID, pAnimInfo->Loop, pAnimInfo->Offset, pAnimInfo->Speed,
							true, AnimPriority_Default, pAnimInfo->Weight);
	}
	else AnimTaskID = INVALID_INDEX;

	pCurrAnimInfo = pAnimInfo;
}
//---------------------------------------------------------------------

void CPropSmartObject::UpdateAnimationCursor()
{
	if (AnimTaskID == INVALID_INDEX || !pCurrAnimInfo || pCurrAnimInfo->Speed == 0.f) return;
	CPropAnimation* pPropAnim = GetEntity()->GetProperty<CPropAnimation>();
	if (!pPropAnim) return;

	float CursorPos = pCurrAnimInfo->Offset + TrProgress * pCurrAnimInfo->Speed;
	if (!pCurrAnimInfo->Loop && pCurrAnimInfo->Duration != TrProgress && TrDuration != 0.f)
		CursorPos *= pCurrAnimInfo->Duration / TrDuration;
	pPropAnim->SetAnimCursorPos(AnimTaskID, CursorPos);
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

bool CPropSmartObject::IsActionAvailable(CStrID ID, const AI::CActor* pActor) const
{
	int Idx = Actions.FindIndex(ID);
	if (Idx == INVALID_INDEX) FAIL;

	const CAction& Action = Actions.ValueAt(Idx);
	if (!Action.Enabled || !Action.FreeUserSlots || !Action.pTpl) FAIL;

	const AI::CSmartAction& ActTpl = *Action.pTpl;
	if (ActTpl.TargetState.IsValid() && IsInTransition() && TrActionID != ID) FAIL;

	//!!!call action function! must not be per-object to avoid duplication!
	//ScriptSrv->RunScript();
	//or
	//PlaceOnStack("Actions"), Run function
	//Or RunFunction("Actions.FuncName")

	Prop::CPropScriptable* pScriptable = GetEntity()->GetProperty<CPropScriptable>();
	if (!pScriptable || !pScriptable->GetScriptObject().IsValid()) return !!pActor; //???or OK?
	Data::CData Args[] = { ID, pActor ? pActor->GetEntity()->GetUID() : CStrID::Empty };
	DWORD Res = pScriptable->GetScriptObject()->RunFunction("IsActionAvailableCallback", Args, 2);
	if (Res == Error_Scripting_NoFunction) return !!pActor; //???or OK?
	return Res == Success;
}
//---------------------------------------------------------------------

bool CPropSmartObject::IsActionAvailableFrom(CStrID ActionID, const vector3& ActorPos) const
{
	//n_assert(false);

	//???use overrides?

	// Nav region ID or nav polys can be specified as a valid action zone
	// If not sppecified, zone is a SO position point only
	// If max radius is defined
	// - if zone, check closest point on poly from zone not farther
	// - if no zone, check distance to SO
	// if min radius is defined
	// - if zone, ignore or use?
	// - if no zone, check distance to SO

	OK;
}
//---------------------------------------------------------------------

bool CPropSmartObject::GetRequiredActorPosition(CStrID ActionID, const AI::CActor* pActor, const vector3& SOPos, vector3& OutPos)
{
	//!!!pass SOFacing!

	//Call action override

	//???Call SO callback?

	//Standard algorithm

	const CAction* pAction = GetAction(ActionID);
	if (!pAction) FAIL;

	//OutMinDist = pAction->pTpl->MinDistance;
	//OutMaxDist = pAction->pTpl->MaxDistance;
	//if (pAction->pTpl->ActorRadiusMatters())
	//{
	//	OutMinDist += pActor->Radius;
	//	OutMaxDist += pActor->Radius;
	//}
	//???add SORadiusMatters? for items, enemies etc

	//OutPos = closest point on the closest reachable poly

	//!!!TMP!
	OutPos = SOPos;

	OK;
}
//---------------------------------------------------------------------

// Assumes that actor is at required position
bool CPropSmartObject::GetRequiredActorFacing(CStrID ActionID, const AI::CActor* pActor, vector3& OutFaceDir)
{
	const CAction* pAction = GetAction(ActionID);
	if (!pAction || !pAction->pTpl->FaceObject()) FAIL;
	OutFaceDir = GetEntity()->GetAttr<matrix44>(CStrID("Transform")).Translation() - pActor->Position;
	OutFaceDir.norm();
	OK;
}
//---------------------------------------------------------------------

}