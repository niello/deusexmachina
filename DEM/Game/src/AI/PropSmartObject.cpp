#include "PropSmartObject.h"

#include <AI/AIServer.h>
#include <AI/SmartObj/SmartAction.h>
#include <AI/PropActorBrain.h> // For GetEntity() only
#include <Scripting/PropScriptable.h>
#include <Events/EventServer.h>
#include <Data/ParamsUtils.h>
#include <Data/DataArray.h>
#include <Math/Math.h>
#include <Core/Factory.h>

namespace Prop
{
FACTORY_CLASS_IMPL(Prop::CPropSmartObject, 'PRSO', Game::CProperty);
__ImplementPropertyStorage(CPropSmartObject);

CPropSmartObject::CPropSmartObject(): Actions(1, 2), Anims(0, 2), ActionAnimIndices(0, 2), StateAnimIndices(0, 2)
{
}
//---------------------------------------------------------------------

CPropSmartObject::~CPropSmartObject()
{
}
//---------------------------------------------------------------------

bool CPropSmartObject::InternalActivate()
{
	Data::PParams Desc;
	const CString& DescResource = GetEntity()->GetAttr<CString>(CStrID("SODesc"), CString::Empty);
	if (DescResource.IsValid())
		Desc = ParamsUtils::LoadParamsFromPRM(CString("Smarts:") + DescResource + ".prm");

	if (Desc.IsValidPtr())
	{
		TypeID = Desc->Get(CStrID("TypeID"), CStrID::Empty);

		Data::PParams ActionsEnabled;
		GetEntity()->GetAttr(ActionsEnabled, CStrID("SOActionsEnabled"));

		Data::PParams DescSection;
		if (Desc->TryGet<Data::PParams>(DescSection, CStrID("Actions")))
		{
			Actions.BeginAdd(DescSection->GetCount());
			for (UPTR i = 0; i < DescSection->GetCount(); ++i)
			{
				const Data::CParam& Prm = DescSection->Get(i);
				const AI::CSmartAction* pTpl = AISrv->GetSmartAction(Prm.GetValue<CStrID>());
				if (pTpl)
				{
					CAction& Action = Actions.Add(Prm.GetName());
					Action.pTpl = pTpl;
					Action.FreeUserSlots = pTpl->MaxUserCount;
					Action.Enabled = ActionsEnabled.IsValidPtr() ? ActionsEnabled->TryGet(Action.Enabled, Prm.GetName()) : false;
				}
				else Sys::Log("AI,SO,Warning: entity '%s', can't find smart object action template '%s'\n",
						GetEntity()->GetUID().CStr(), Prm.GetValue<CStrID>().CStr());
			}
			Actions.EndAdd();
		}
	}

	//AnimTaskID = INVALID_INDEX;
	//pCurrAnimInfo = nullptr;

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

		// Make transition from nullptr state to DefaultState
		if (!TargetState.IsValid() && Desc.IsValidPtr() && Desc->Has(CStrID("DefaultState")))
			PROP_SUBSCRIBE_PEVENT(OnPropsActivated, CPropSmartObject, OnPropsActivated);
	}

	// Make sure the attribute is set
	GetEntity()->SetAttr(CStrID("SOState"), CurrState);

	CPropScriptable* pPropScript = GetEntity()->GetProperty<CPropScriptable>();
	if (pPropScript && pPropScript->IsActive()) EnableSI(*pPropScript);

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

	SwitchAnimation(nullptr);

	//???delete related entity attrs?

	CurrState = CStrID::Empty;
	ActionAnimIndices.Clear();
	StateAnimIndices.Clear();
	Anims.Clear();
	Actions.Clear();
}
//---------------------------------------------------------------------

bool CPropSmartObject::OnPropsActivated(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	// Initialize current state and transition.
	// Do it here to make sure that script is loaded and will process transition events.
	if (TargetState.IsValid())
		SetState(TargetState, TrActionID, TrDuration, TrManualControl);
	else
	{
		const CString& DescResource = GetEntity()->GetAttr<CString>(CStrID("SODesc"), CString::Empty);
		Data::PParams Desc = ParamsUtils::LoadParamsFromPRM(CString("Smarts:") + DescResource + ".prm");
		SetState(Desc->Get<CStrID>(CStrID("DefaultState"), CStrID::Empty), TrActionID, -1.f, TrManualControl);
	}
	OK;
}
//---------------------------------------------------------------------

bool CPropSmartObject::OnPropActivated(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropScriptable>())
	{
		EnableSI(*(CPropScriptable*)pProp);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropSmartObject::OnPropDeactivating(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
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

bool CPropSmartObject::OnLevelSaving(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
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
	for (UPTR i = 0; i < Actions.GetCount(); ++i)
		P->Set(Actions.KeyAt(i), Actions.ValueAt(i).Enabled);
	GetEntity()->SetAttr(CStrID("SOActionsEnabled"), P);
	OK;
}
//---------------------------------------------------------------------

bool CPropSmartObject::OnBeginFrame(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	float Time = 0.f;//(float)GameSrv->GetFrameTime();
	if (Time != 0.f) SetTransitionProgress(TrProgress + Time);
	OK;
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
		//	and make params here looking as (x->y)->y
		//else
		AbortTransition();

		//!!!if not bidirectional, exit (x->y)->x here because it becomes (x->x)->x!
	}

	IPTR Idx = ActionAnimIndices.FindIndex(ActionID);
	CAnimInfo* pAnimInfo = (Idx != INVALID_INDEX) ? &Anims[ActionAnimIndices.ValueAt(Idx)] : nullptr;
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

	IPTR Idx = StateAnimIndices.FindIndex(TargetState);
	SwitchAnimation((Idx != INVALID_INDEX) ? &Anims[StateAnimIndices.ValueAt(Idx)] : nullptr);

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
	TrProgress = std::clamp(Time, 0.f, TrDuration);
	UpdateAnimationCursor();
	if (TrProgress >= TrDuration) CompleteTransition();
}
//---------------------------------------------------------------------

void CPropSmartObject::StopTransition()
{
	UNSUBSCRIBE_EVENT(OnBeginFrame);
	TrManualControl = true;
}
//---------------------------------------------------------------------

void CPropSmartObject::AbortTransition(float Duration)
{
	//!!!implement nonzero duration!
	n_assert2(Duration == 0.f, "Implement nonzero duration!!!");

	//???interchange curr and target and complete transition?
	//???only if bidirectional transition available for this state pair?

	IPTR Idx = StateAnimIndices.FindIndex(CurrState);
	SwitchAnimation((Idx != INVALID_INDEX) ? &Anims[StateAnimIndices.ValueAt(Idx)] : nullptr);

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
}
//---------------------------------------------------------------------

void CPropSmartObject::UpdateAnimationCursor()
{
	if (AnimTaskID == INVALID_INDEX || !pCurrAnimInfo || pCurrAnimInfo->Speed == 0.f) return;

	float CursorPos = pCurrAnimInfo->Offset + TrProgress * pCurrAnimInfo->Speed;
	if (!pCurrAnimInfo->Loop && pCurrAnimInfo->Duration != TrProgress && TrDuration != 0.f)
		CursorPos *= pCurrAnimInfo->Duration / TrDuration;
}
//---------------------------------------------------------------------

void CPropSmartObject::EnableAction(CStrID ID, bool Enable)
{
	IPTR Idx = Actions.FindIndex(ID);
	if (Idx == INVALID_INDEX) return;

	CAction& Action = Actions.ValueAt(Idx);
	if (Action.Enabled == Enable) return;
	Action.Enabled = Enable;

	Data::PParams P = n_new(Data::CParams(2));
	P->Set(CStrID("ActionID"), ID);
	P->Set(CStrID("Enabled"), Enable);
	GetEntity()->FireEvent(CStrID("OnSOActionAvailable"), P);
}
//---------------------------------------------------------------------

bool CPropSmartObject::IsActionAvailable(CStrID ID, const AI::CActor* pActor) const
{
	IPTR Idx = Actions.FindIndex(ID);
	if (Idx == INVALID_INDEX) FAIL;

	const CAction& Action = Actions.ValueAt(Idx);
	if (!Action.Enabled || !Action.FreeUserSlots || !Action.pTpl) FAIL;

	const AI::CSmartAction& ActTpl = *Action.pTpl;
	if (ActTpl.TargetState.IsValid() && IsInTransition() && TrActionID != ID) FAIL;

	if (!ActTpl.IsValid(pActor ? pActor->GetEntity()->GetUID() : CStrID::Empty, GetEntity()->GetUID())) FAIL;

	Prop::CPropScriptable* pScriptable = GetEntity()->GetProperty<CPropScriptable>();
	if (!pScriptable || pScriptable->GetScriptObject().IsNullPtr()) return !!pActor; //???or OK?
	Data::CData Args[] = { ID, pActor ? pActor->GetEntity()->GetUID() : CStrID::Empty };
	UPTR Res = pScriptable->GetScriptObject()->RunFunction("IsActionAvailableCallback", Args, 2);
	if (Res == Error_Scripting_NoFunction) return !!pActor; //???or OK?
	return Res == Success;
}
//---------------------------------------------------------------------

// Smart object is smart because it can describe how to use it. Position prediction is handled
// here too, though it may look like actor class should do this. The reason is that only a smart
// object itself knows, when prediction against it is necessary.
// Navigation cache stores polys around the object. It can be used for writing, when the first call
// is made, or for reading in the subsequent calls, if SO didn't move since the cache was filled.
bool CPropSmartObject::GetRequiredActorPosition(CStrID ActionID, const AI::CActor* pActor, vector3& OutPos,
												CArray<dtPolyRef>* pNavCache, bool UpdateCache)
{
//!!!predict and use facing!

	const CAction* pAction = GetAction(ActionID);
	if (!pAction) FAIL;

	//!!!call overrides here!
	// override may use nav region or even something completely different
	//clear cache if specified and update needed

	const matrix44& Tfm = GetEntity()->GetAttr<matrix44>(CStrID("Transform"));
	vector3 Pos = Tfm.Translation();

	vector3 Vel;
	//!!!if (pActor->TargetPosPredictionMode != None && ...)
	if (GetEntity()->GetAttr<vector3>(Vel, CStrID("LinearVelocity")) && Vel.SqLength2D() > 0.f)
	{
		vector3 Dist = Pos - pActor->Position;
		float MaxSpeed = 0.f; //pActor->GetMotorSystem().GetMaxSpeed();
		float Time;

		if (true) // pActor->TargetPosPredictionMode == Quadratic
		{
			// Quadratic firing solution
			float A = Vel.SqLength2D() - MaxSpeed * MaxSpeed;
			float B = 2.f * (Dist.x * Vel.x + Dist.z * Vel.z);
			float C = Dist.SqLength2D();
			float Time1, Time2;

			UPTR RootCount = Math::SolveQuadraticEquation(A, B, C, &Time1, &Time2);
			if (!RootCount) FAIL; //!!!keep a prev point some time! or use current pos?
			Time = (RootCount == 1 || (Time1 < Time2 && Time1 > 0.f)) ? Time1 : Time2;

			//???is possible?
			if (Time < 0.f) FAIL; //!!!keep a prev point some time! or use current pos?
		}
		else
		{
			// Pursue steering
			Time = Dist.Length2D() / MaxSpeed;
		}

		//???need? see if it adds realism!
		//const float MaxPredictionTime = 5.f; //???to settings?
		//if (Time > MaxPredictionTime) Time = MaxPredictionTime;

		Pos.x += Vel.x * Time;
		Pos.z += Vel.z * Time;

		//!!!assumed that a moving SO with a relatively high speed (no short step!) tries
		//to face to where it moves, predict facing smth like this:
		// get angle between curr facing and velocity direction
		// get maximum angle as Time * MaxAngularSpeed
		// get min(angle_between, max_angle) and rotate curr direction towards a velocity direction
		// this will be a predicted facing
		// may be essential for AI-made backstabs
	}

	//!!!Apply local dest offset now, using predicted pos & facing!

	float MinRange = pAction->pTpl->MinRange;
	float MaxRange = pAction->pTpl->MaxRange;
	if (pAction->pTpl->ActorRadiusMatters())
	{
		MinRange += pActor->Radius;
		MaxRange += pActor->Radius;
	}
	//if (pAction->pTpl->SORadiusMatters())
	//{
	//	MinRange += GetAttr(Radius);
	//	MaxRange += GetAttr(Radius);
	//}

	if (pActor->IsAtPoint(Pos, MinRange, MaxRange))
	{
		OutPos = pActor->Position;
		OK;
	}

	/*
	if (pNavCache)
	{
		for (UPTR i = 0; i < pNavCache->GetCount(); ++i)
			if (!pActor->GetNavSystem().IsPolyValid(pNavCache->At(i)))
			{
				UpdateCache = true;
				break;
			}

		if (UpdateCache && !pActor->GetNavSystem().GetValidPolys(Pos, MinRange, MaxRange, *pNavCache)) FAIL;
		if (!pActor->GetNavSystem().GetNearestValidLocation(pNavCache->Begin(), pNavCache->GetCount(), Pos, MinRange, MaxRange, OutPos)) FAIL;
	}
	else
	{
		if (!pActor->GetNavSystem().GetNearestValidLocation(Pos, MinRange, MaxRange, OutPos)) FAIL;
	}
	*/

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

CPropSmartObject::CAction* CPropSmartObject::GetAction(CStrID ID)
{
	IPTR Idx = Actions.FindIndex(ID);
	return (Idx != INVALID_INDEX) ? &Actions.ValueAt(Idx) : nullptr;
}
//---------------------------------------------------------------------

const CPropSmartObject::CAction* CPropSmartObject::GetAction(CStrID ID) const
{
	IPTR Idx = Actions.FindIndex(ID);
	return (Idx != INVALID_INDEX) ? &Actions.ValueAt(Idx) : nullptr;
}
//---------------------------------------------------------------------

bool CPropSmartObject::IsActionEnabled(CStrID ID) const
{
	IPTR Idx = Actions.FindIndex(ID);
	if (Idx == INVALID_INDEX) FAIL;
	const CAction& Action = Actions.ValueAt(Idx);
	return Action.Enabled && Action.pTpl;
}
//---------------------------------------------------------------------

}
