#include "PropActorBrain.h"

#include <AI/PropSmartObject.h>
#include <AI/AIServer.h>
#include <AI/Behaviour/Action.h>
#include <Physics/PropCharacterController.h>
#include <Scripting/PropScriptable.h>
#include <Data/ParamsUtils.h>
#include <Data/DataArray.h>
#include <Game/GameServer.h>
#include <Game/GameLevel.h>
#include <Core/Factory.h>

namespace Prop
{
__ImplementClass(Prop::CPropActorBrain, 'PRAB', Game::CProperty);
__ImplementPropertyStorage(CPropActorBrain);

using namespace Data;

static const CString StrPercPrefix("AI::CPerceptor");
static const CString StrSensorPrefix("AI::CSensor");
static const CString StrGoalPrefix("AI::CGoal");

struct CGoalCmp_RelevanceDescending
{
	inline bool operator()(const PGoal& R1, const PGoal& R2) const
	{
		return R1->GetRelevance() > R2->GetRelevance();
	}
};

// At very small speeds and physics step sizes body position stops updating because of limited
// float precision. LinearSpeed = 0.0007f, StepSize = 0.01f, Pos + LinearSpeed * StepSize = Pos.
// So body never reaches the desired destination and we must accept arrival at given tolerance.
// The less is this value, the more precise is resulting position, but the more time arrival takes.
// Empirical minimum value is somewhere around 0.0008f.
// This value is measured in game world meters.
// NB: may be square of this value MUST make sense with a single (float) precision.
const float CPropActorBrain::LinearArrivalTolerance = 0.009f;

// In radians
const float CPropActorBrain::AngularArrivalTolerance = 0.005f;

bool CPropActorBrain::InternalActivate()
{
	if (!GetEntity()->GetLevel()->GetAI()) FAIL;

// Blackboard

	const matrix44& Tfm = GetEntity()->GetAttr<matrix44>(CStrID("Transform"));
	Position = Tfm.Translation();
	LookatDir = -Tfm.AxisZ();

	//!!!update on R/H attrs change!
	Radius = GetEntity()->GetAttr<float>(CStrID("Radius"), 0.3f);
	Height = GetEntity()->GetAttr<float>(CStrID("Height"), 1.75f);

	NavState = AINav_Done;
	SetNavLocationValid(false);
	SetAcceptNearestValidDestination(false);

	MvmtState = AIMvmt_Done;
	MvmtType = AIMvmt_Type_Walk;
	SteeringType = AISteer_Type_Seek;
	FacingState = AIFacing_Done;

// END Blackboard

	//???need to cache?
	PParams Desc;
	if (ParamsUtils::LoadDescFromPRM(CString("Actors:"), GetEntity()->GetAttr<CString>(CStrID("ActorDesc")) + ".prm", Desc))
	{
		PParams DescSection;
		if (Desc->Get<PParams>(DescSection, CStrID("Perceptors")))
		{
			Perceptors.Reallocate(DescSection->GetCount(), 0);
			for (UPTR i = 0; i < DescSection->GetCount(); ++i)
			{
				const CParam& DescParam = DescSection->Get(i);
				PPerceptor New = (CPerceptor*)Factory->Create(StrPercPrefix + DescParam.GetName().CStr());
				New->Init(*DescParam.GetValue<PParams>());
				Perceptors.Add(New);
			}
		}

		if (Desc->Get<PParams>(DescSection, CStrID("Sensors")))
		{
			Sensors.Reallocate(DescSection->GetCount(), 0);
			for (UPTR i = 0; i < DescSection->GetCount(); ++i)
			{
				const CParam& DescParam = DescSection->Get(i);
				PSensor New = (CSensor*)Factory->Create(StrSensorPrefix + DescParam.GetName().CStr());
				PParams NewDesc = DescParam.GetValue<PParams>();
				New->Init(*NewDesc);
				Sensors.Add(New);
				
				PDataArray Percs = NewDesc->Get<PDataArray>(CStrID("Perceptors"), NULL);
				if (Percs.IsValidPtr())
				{
					CDataArray::CIterator ItPercName;
					for (ItPercName = Percs->Begin(); ItPercName != Percs->End(); ++ItPercName)
					{
						CString PercClass = StrPercPrefix + ItPercName->GetValue<CString>();
						bool Found = false;

						CArray<PPerceptor>::CIterator ItPerc;
						for (ItPerc = Perceptors.Begin(); ItPerc != Perceptors.End(); ++ItPerc)
						{
							//???store class name as CStrID to compare faster?
							if ((*ItPerc)->GetClassName() == PercClass)
							{
								New->AddPerceptor(*ItPerc);
								Found = true;
								break;
							}
						}

						if (!Found)
							Sys::Log("AI,Warning: CPropActorBrain::InternalActivate() > entity '%s', perceptor '%s' not found\n",
									GetEntity()->GetUID().CStr(),
									ItPercName->GetValue<CString>().CStr());
					}
				}
			}
		}
		
		if (Desc->Get<PParams>(DescSection, CStrID("Goals")))
		{
			//int HasIdleGoal = DescSection->Has(CStrID("Idle")) ? 1 : 0;

			Goals.Reallocate(DescSection->GetCount() /*+ 1 - HasIdleGoal*/, 0);
			for (UPTR i = 0; i < DescSection->GetCount(); ++i)
			{
				const CParam& DescParam = DescSection->Get(i);
				PGoal New = (CGoal*)Factory->Create(StrGoalPrefix + DescParam.GetName().CStr());
				New->Init(DescParam.GetValue<PParams>());
				Goals.Add(New);
			}

			//if (!HasIdleGoal)
			//{
			//	PGoal New = n_new(CGoalIdle);
			//	New->Init(NULL);
			//	Goals.Add(New);
			//}
		}

		PDataArray ActionArray;
		if (Desc->Get<PDataArray>(ActionArray, CStrID("Actions")))
		{
			ActionTpls.Reallocate(ActionArray->GetCount(), 0);
			for (UPTR i = 0; i < ActionArray->GetCount(); ++i)
			{
				const char* pActionName = ActionArray->At(i).GetValue<CString>().CStr();
				const CActionTpl* pTpl = AISrv->GetPlanner().FindActionTpl(pActionName);
				if (pTpl) ActionTpls.Add(pTpl);
				else Sys::Log("AI,Warning: entity '%s' requested action template '%s' that is not registered\n",
						GetEntity()->GetUID().CStr(), pActionName);
			}
		}

		bool DecMaking = Desc->Get(CStrID("EnableDecisionMaking"), false);
		Flags.SetTo(AIMind_EnableDecisionMaking, DecMaking);
		Flags.SetTo(AIMind_SelectAction, DecMaking);

		NavDestRecoveryTime = Desc->Get(CStrID("NavDestRecoveryTime"), 3.f);
		
		NavSystem.Init(Desc->Get<PParams>(CStrID("Navigation"), NULL).GetUnsafe());
		MotorSystem.Init(Desc->Get<PParams>(CStrID("Movement"), NULL).GetUnsafe());
	}

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive()) EnableSI(*pProp);

	PROP_SUBSCRIBE_PEVENT(OnBeginFrame, CPropActorBrain, OnBeginFrame);
	PROP_SUBSCRIBE_PEVENT(OnRenderDebug, CPropActorBrain, OnRenderDebug);
	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropActorBrain, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnPropDeactivating, CPropActorBrain, OnPropDeactivating);
	PROP_SUBSCRIBE_PEVENT(UpdateTransform, CPropActorBrain, OnUpdateTransform);
	PROP_SUBSCRIBE_PEVENT_PRIORITY(BeforePhysicsTick, CPropActorBrain, BeforePhysicsTick, 20);
	PROP_SUBSCRIBE_PEVENT(AfterPhysicsTick, CPropActorBrain, AfterPhysicsTick);
	PROP_SUBSCRIBE_PEVENT(OnNavMeshDataChanged, CPropActorBrain, OnNavMeshDataChanged);
	OK;
}
//---------------------------------------------------------------------

void CPropActorBrain::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(OnBeginFrame);
	UNSUBSCRIBE_EVENT(OnRenderDebug);
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnPropDeactivating);
	UNSUBSCRIBE_EVENT(UpdateTransform);
	UNSUBSCRIBE_EVENT(BeforePhysicsTick);
	UNSUBSCRIBE_EVENT(AfterPhysicsTick);
	UNSUBSCRIBE_EVENT(OnNavMeshDataChanged);

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive()) DisableSI(*pProp);

	NavSystem.Term();
}
//---------------------------------------------------------------------

bool CPropActorBrain::OnPropActivated(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
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

bool CPropActorBrain::OnPropDeactivating(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
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

void CPropActorBrain::EnqueueTask(const CTask& Task)
{
	if (Task.Plan.IsNullPtr()) return;
	bool WasEmpty = TaskQueue.IsEmpty();
	TaskQueue.AddBack(Task);
	if (WasEmpty) RequestBehaviourUpdate();
}
//---------------------------------------------------------------------

void CPropActorBrain::ClearTaskQueue()
{
	if (TaskQueue.IsEmpty()) return;
	TaskQueue.Clear();
	RequestBehaviourUpdate();
}
//---------------------------------------------------------------------

void CPropActorBrain::AbortCurrAction(UPTR Result)
{
	SetPlan(NULL, CurrGoal.GetUnsafe(), Running);
	RequestBehaviourUpdate();
}
//---------------------------------------------------------------------

void CPropActorBrain::SetPlan(PAction NewPlan, CGoal* pPrevGoal, UPTR PrevPlanResult)
{
	if (Plan == NewPlan) return;

	CTask* pTask = TaskQueue.IsEmpty() ? NULL : &TaskQueue.Front();

	if (Plan.IsValidPtr())
	{
		Plan->Deactivate(this);

		if (pTask && Plan == pTask->Plan) // Task was active and now is not
		{
			//!!!deactivate task (abort)!
			//???event task done/aborted? use task UID?
			if (NewPlan.IsNullPtr() || pTask->FailOnInterruption) // Valid NewPlan means an interruption
			{
				if (NewPlan.IsValidPtr() && pTask->FailOnInterruption) PrevPlanResult = Failure;
				if (PrevPlanResult == Failure && pTask->ClearQueueOnFailure) TaskQueue.Clear();
				else TaskQueue.RemoveFront();
			}
		}
		else if (CurrGoal.GetUnsafe() != pPrevGoal) // Goal was active and now is not
		{
			n_assert(pPrevGoal); // Prevoius plan is not set by task, so it MUST have been set by goal
			// [Deactivate pCurrGoal]
		}
	}

	Plan = NewPlan;

	if (Plan.IsValidPtr())
	{
		if (pTask && Plan == pTask->Plan) // Task becomes active
		{
			//!!!activate task!
			//???event task activated? use task UID?
		}
		else if (CurrGoal.GetUnsafe() != pPrevGoal) // Goal becomes active
		{
			n_assert(CurrGoal.IsValidPtr()); // New plan is not set by task, so it MUST have been set by goal
			// [Activate CurrGoal]
		}

		if (!Plan->Activate(this))
		{
			Flags.Set(AIMind_InvalidatePlan);
			Plan = NULL;
		}
	}
}
//---------------------------------------------------------------------

void CPropActorBrain::UpdateBehaviour()
{
	bool NeedToReplan =
		Flags.Is(AIMind_InvalidatePlan) ||
		(CurrGoal.IsValidPtr() && (Plan.IsNullPtr() || CurrGoal->IsSatisfied()));

	if (!NeedToReplan && Flags.IsNot(AIMind_SelectAction)) return;

	Flags.Clear(AIMind_SelectAction | AIMind_InvalidatePlan);

	CGoal* pPrevGoal = CurrGoal.GetUnsafe();
	CTask* pTask = TaskQueue.IsEmpty() ? NULL : &TaskQueue.Front();

	//???!!!check can task action be interrupted, if task active?

	PAction NewPlan;

	// Disable this flag for player- or script-controlled actors
	if (Flags.Is(AIMind_EnableDecisionMaking))
	{
		for (CArray<PGoal>::CIterator ppGoal = Goals.Begin(); ppGoal != Goals.End(); ++ppGoal)
		{
			//???all this to EvalRel?
			// If not a time yet, skip goal & invalidate its relevance
			// If satisfied actor's WS and not reeval on satisfaction, invalidate
			(*ppGoal)->EvalRelevance(this);
			// Set new recalc time
		}

		Goals.Sort<CGoalCmp_RelevanceDescending>();

		// We search for the valid goal or task with the highest priority
		// When relevances are equal, prefer in order CurrentGoal->Task->Goal
		float CurrGoalRelevance = pPrevGoal ? pPrevGoal->GetRelevance() : 0.f;
		float TaskRelevance = pTask ? pTask->Relevance : 0.f;
		bool PreferTask = (CurrGoalRelevance < TaskRelevance);

		for (CArray<PGoal>::CIterator ppGoal = Goals.Begin(); ppGoal != Goals.End(); ++ppGoal)
		{
			CGoal* pTopGoal = *ppGoal;

			// Since we always prefer the current goal, we do this before comparing relevances
			if (pPrevGoal == pTopGoal && !NeedToReplan && !pPrevGoal->IsReplanningNeeded())
			{
				n_assert_dbg(Plan.IsValidPtr());
				NewPlan = Plan;
				break;
			}

			// If need to interrupt, but can't, continue
			// [Test against activation probability, if fails, continue]

			float Relevance = pTopGoal->GetRelevance();
			if (Relevance < TaskRelevance || (Relevance == TaskRelevance && PreferTask)) break;

			NewPlan = AISrv->GetPlanner().BuildPlan(this, pTopGoal);
			if (NewPlan.IsValidPtr())
			{
				CurrGoal = pTopGoal;
				break;
			}

			//!!!HandleBuildPlanFailure() with ddefault impl as InvalidateRelevance()!
			pTopGoal->InvalidateRelevance();
		}

		//???n_assert2(CurrGoal.GetUnsafe() || pTask, "Actor has no goal, even GoalIdle, nor task");
	}

	if (NewPlan.IsNullPtr())
	{
		CurrGoal = NULL;
		if (pTask)
		{
			NewPlan = pTask->Plan;
			n_assert_dbg(NewPlan.IsValidPtr());
		}
	}

	SetPlan(NewPlan, pPrevGoal, Running);
}
//---------------------------------------------------------------------

bool CPropActorBrain::OnBeginFrame(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	float FrameTime = (float)GameSrv->GetFrameTime();

	//!!!update for some time, if not updated all return or allow to process next!
	//!!!only for external (tests external stimuli placed in the level)!
	//also need to update internal sensors & actor's state through them
	//???CStimulus -> CExternalStimulus?
	for (CArray<PSensor>::CIterator ppSensor = Sensors.Begin(); ppSensor != Sensors.End(); ++ppSensor)
		GetEntity()->GetLevel()->GetAI()->UpdateActorSense(this, (*ppSensor));

	MemSystem.Update();

#ifndef _EDITOR
	UpdateBehaviour();

	NavSystem.Update(FrameTime);
#endif

	if (Plan.IsValidPtr())
	{
		UPTR PlanResult = Plan->IsValid(this) ? Plan->Update(this) : Failure;
		if (PlanResult != Running)
		{
			SetPlan(NULL, CurrGoal.GetUnsafe(), PlanResult);
			RequestBehaviourUpdate();
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool CPropActorBrain::OnUpdateTransform(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const matrix44& Tfm = GetEntity()->GetAttr<matrix44>(CStrID("Transform"));
	LookatDir = -Tfm.AxisZ();
	if (Position != Tfm.Translation())
	{
		Position = Tfm.Translation();
		NavSystem.UpdatePosition();
		MotorSystem.UpdatePosition();
	}

	OK;
}
//---------------------------------------------------------------------

bool CPropActorBrain::BeforePhysicsTick(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
#ifndef _EDITOR
	MotorSystem.Update(((const Events::CEvent&)Event).Params->Get<float>(CStrID("FrameTime")));
#endif
	OK;
}
//---------------------------------------------------------------------

// We need to react on subframe transform changes to drive physics correctly
bool CPropActorBrain::AfterPhysicsTick(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	//???or access node controller directly and read subframe data?
	Prop::CPropCharacterController* pCC = GetEntity()->GetProperty<Prop::CPropCharacterController>();
	if (!pCC) OK;

	//!!!FIXME! write to the Bullet support:
	// It is strange, but post-tick callback is called before synchronizeMotionStates(), so the body
	// has an outdated transformation here. So we have to access object's world tfm.
	vector3 OldPos = Position;
	quaternion Rot;
	pCC->GetController()->GetBody()->Physics::CPhysicsObject::GetTransform(Position, Rot); //!!!need nonvirtual method "GetWorld/PhysicsTransform"!
	LookatDir = Rot.rotate(vector3::BaseDir);

	if (OldPos != Position)
	{
		NavSystem.UpdatePosition();
		MotorSystem.UpdatePosition();
	}

	OK;
}
//---------------------------------------------------------------------

void CPropActorBrain::FillWorldState(CWorldState& WSCurr) const
{
	//!!!!fill with real status!
	WSCurr.SetProp(WSP_AtEntityPos, CStrID::Empty);
	WSCurr.SetProp(WSP_UsingSmartObj, CStrID::Empty);
	WSCurr.SetProp(WSP_Action, CStrID::Empty);
	WSCurr.SetProp(WSP_HasItem, CStrID::Empty);
	WSCurr.SetProp(WSP_ItemEquipped, CStrID::Empty);
}
//---------------------------------------------------------------------

bool CPropActorBrain::OnNavMeshDataChanged(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	//!!!test when actor goes somewhere!
	NavSystem.SetupState();
	RequestBehaviourUpdate();
	OK;
}
//---------------------------------------------------------------------

bool CPropActorBrain::OnRenderDebug(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	if (GetEntity()->GetUID() == "GG") //!!!write debug focus or smth!
	{
		NavSystem.RenderDebug();
		MotorSystem.RenderDebug();
	}
	OK;
}
//---------------------------------------------------------------------

}