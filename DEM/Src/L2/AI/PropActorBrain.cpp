#include "PropActorBrain.h"

#include <AI/PropSmartObject.h>
#include <AI/AIServer.h>
#include <AI/Behaviour/Action.h>
#include <AI/Planning/GoalIdle.h>
#include <AI/Events/QueueTask.h>
#include <Physics/PropCharacterController.h>
#include <Scripting/PropScriptable.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <Game/EntityManager.h>
#include <Game/GameServer.h>

#ifdef __WIN32__
	#undef GetClassName
	#ifdef GetProp
		#undef GetProp
		#undef SetProp
	#endif
#endif

namespace Prop
{
__ImplementClass(Prop::CPropActorBrain, 'PRAB', Game::CProperty);
__ImplementPropertyStorage(CPropActorBrain);

using namespace Data;

static const CString StrPercPrefix("AI::CPerceptor");
static const CString StrSensorPrefix("AI::CSensor");
static const CString StrGoalPrefix("AI::CGoal");

// At very small speeds and physics step sizes body position stops updating because of limited
// float precision. LinearSpeed = 0.0007f, StepSize = 0.01f, Pos + LinearSpeed * StepSize = Pos.
// So body never reaches the desired destination and we must accept arrival at given tolerance.
// The less is this value, the more precise is resulting position, but the more time arrival takes.
// Empirical minimum value is somewhere around 0.0008f.
// This value is measured in game world meters.
// NB: may be square of this value MUST make sense with a single (float) precision too.
const float CPropActorBrain::ArrivalTolerance = 0.009f;

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
	AcceptNearestValidDestination(false);

	MvmtState = AIMvmt_Done;
	MvmtType = AIMvmt_Type_Walk;
	SteeringType = AISteer_Type_Seek;
	FacingState = AIFacing_Done;

// END Blackboard
	
	static const CString DefaultState("Default");

	//???need to cache?
	PParams Desc;
	if (DataSrv->LoadDesc(Desc, "Actors:", GetEntity()->GetAttr<CString>(CStrID("ActorDesc"))))
	{
		PParams DescSection;
		if (Desc->Get<PParams>(DescSection, CStrID("Perceptors")))
		{
			Perceptors.Reallocate(DescSection->GetCount(), 0);
			for (int i = 0; i < DescSection->GetCount(); i++)
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
			for (int i = 0; i < DescSection->GetCount(); i++)
			{
				const CParam& DescParam = DescSection->Get(i);
				PSensor New = (CSensor*)Factory->Create(StrSensorPrefix + DescParam.GetName().CStr());
				PParams NewDesc = DescParam.GetValue<PParams>();
				New->Init(*NewDesc);
				Sensors.Add(New);
				
				PDataArray Percs = NewDesc->Get<PDataArray>(CStrID("Perceptors"), NULL);
				if (Percs.IsValid())
				{
					CDataArray::CIterator ItPercName;
					for (ItPercName = Percs->Begin(); ItPercName != Percs->End(); ItPercName++)
					{
						CString PercClass = StrPercPrefix + ItPercName->GetValue<CString>();
						bool Found = false;

						CArray<PPerceptor>::CIterator ItPerc;
						for (ItPerc = Perceptors.Begin(); ItPerc != Perceptors.End(); ItPerc++)
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
							n_printf("Warning, AI: perceptor '%s' not found in '%s' on activation\n",
									 ItPercName->GetValue<CString>().CStr(),
									 GetEntity()->GetUID().CStr());
					}
				}
			}
		}
		
		if (Desc->Get<PParams>(DescSection, CStrID("Goals")))
		{
			int HasIdleGoal = DescSection->Has(CStrID("Idle")) ? 1 : 0;

			Goals.Reallocate(DescSection->GetCount() + 1 - HasIdleGoal, 0);
			for (int i = 0; i < DescSection->GetCount(); i++)
			{
				const CParam& DescParam = DescSection->Get(i);
				PGoal New = (CGoal*)Factory->Create(StrGoalPrefix + DescParam.GetName().CStr());
				New->Init(DescParam.GetValue<PParams>());
				Goals.Add(New);
			}

			if (!HasIdleGoal)
			{
				PGoal New = n_new(CGoalIdle);
				New->Init(NULL);
				Goals.Add(New);
			}
		}

		PDataArray ActionArray;
		if (Desc->Get<PDataArray>(ActionArray, CStrID("Actions")))
		{
			Actions.Reallocate(ActionArray->GetCount(), 0);
			for (int i = 0; i < ActionArray->GetCount(); i++)
			{
				LPCSTR pActionName = ActionArray->At(i).GetValue<CString>().CStr();
				const CActionTpl* pTpl = AISrv->GetPlanner().FindActionTpl(pActionName);
				if (pTpl) Actions.Add(pTpl);
				else n_printf("Warning, AI: action template '%s' is not registered\n", pActionName);
			}
		}

		bool DecMaking = Desc->Get<bool>(CStrID("EnableDecisionMaking"), false);
		Flags.SetTo(AIMind_EnableDecisionMaking, DecMaking);
		Flags.SetTo(AIMind_UpdateGoal, DecMaking);
		
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
	PROP_SUBSCRIBE_NEVENT(QueueTask, CPropActorBrain, OnAddTask);
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
	UNSUBSCRIBE_EVENT(QueueTask);
	UNSUBSCRIBE_EVENT(OnNavMeshDataChanged);

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive()) DisableSI(*pProp);

	NavSystem.Term();
}
//---------------------------------------------------------------------

bool CPropActorBrain::OnPropActivated(const Events::CEventBase& Event)
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

bool CPropActorBrain::OnPropDeactivating(const Events::CEventBase& Event)
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

bool CPropActorBrain::SetPlan(CAction* pNewPlan)
{
	if (CurrPlan.IsValid()) CurrPlan->Deactivate(this);
	//???deactivate and clear current goal, if has?
	CurrPlan = pNewPlan;
	if (pNewPlan && !pNewPlan->Activate(this))
	{
		Flags.Set(AIMind_InvalidatePlan);
		CurrPlan = NULL;
		FAIL;
	}
	else Flags.Set(AIMind_UpdateGoal); //???why need if valid plan set & activated?

	OK;
}
//---------------------------------------------------------------------

void CPropActorBrain::UpdateDecisionMaking()
{
	bool NeedToReplan =
		Flags.Is(AIMind_InvalidatePlan) ||
		(CurrGoal.IsValid() && (!CurrPlan.IsValid() || CurrGoal->IsSatisfied()));
	bool UpdateGoals = Flags.Is(AIMind_UpdateGoal) || NeedToReplan;

	Flags.Clear(AIMind_UpdateGoal | AIMind_InvalidatePlan);

	if (!UpdateGoals) return;

	for (CArray<PGoal>::CIterator ppGoal = Goals.Begin(); ppGoal != Goals.End(); ++ppGoal)
	{
		//???all this to EvalRel?
		// If not a time still, skip goal & invalidate its relevance
		// If satisfied actor's WS and not reeval on satisfaction, invalidate
		(*ppGoal)->EvalRelevance(this);
		// Set new recalc time
	}

	//???!!!add some constant value to curr goal's relevance to avoid too frequent changes?!

	//???or sort by priority?
	while (true)
	{
		CGoal* pTopGoal = CurrGoal.GetUnsafe();
		float MaxRelevance = CurrGoal.IsValid() ? CurrGoal->GetRelevance() : 0.f;

		for (CArray<PGoal>::CIterator ppGoal = Goals.Begin(); ppGoal != Goals.End(); ++ppGoal)
			if ((*ppGoal)->GetRelevance() > MaxRelevance)
			{
				MaxRelevance = (*ppGoal)->GetRelevance();
				pTopGoal = (*ppGoal).GetUnsafe();
			}

		n_assert2(pTopGoal, "Actor has no goal, even GoalIdle");

		if (CurrGoal.GetUnsafe() == pTopGoal && !NeedToReplan && !CurrGoal->IsReplanningNeeded()) break;

		// If need to interrupt, but can't, invalidate relevance and continue
		// [Test against probability, if fails, invalidate relevance and continue]

		PAction Plan = AISrv->GetPlanner().BuildPlan(this, pTopGoal);
		if (Plan.IsValid())
		{
			//???!!!use SetPlan here?

			if (CurrPlan.IsValid()) CurrPlan->Deactivate(this);

			// [Deactivate curr goal]
			CurrGoal = pTopGoal;
			// [Activate curr goal]

			CurrPlan = Plan;
			if (!CurrPlan->Activate(this))
			{
				Flags.Set(AIMind_InvalidatePlan);
				CurrPlan = NULL;
			}
			break;
		}
		else
		{
			//!!!HandleBuildPlanFailure!
			pTopGoal->InvalidateRelevance();
		}
	}
}
//---------------------------------------------------------------------

bool CPropActorBrain::OnBeginFrame(const Events::CEventBase& Event)
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
	// Disable this flag for player- or script-controlled actors
	//???mb some goals can abort command (as interrupt behaviours)? so check/clear cmd inside in interruption
	if (Flags.Is(AIMind_EnableDecisionMaking) && !CurrTask.IsValid()) UpdateDecisionMaking();

	NavSystem.Update(FrameTime);
#endif

	if (CurrPlan.IsValid())
	{
		DWORD BhvResult = CurrPlan->IsValid(this) ? CurrPlan->Update(this) : Failure;
		if (BhvResult != Running)
		{
			SetPlan(NULL);
			if (CurrTask.IsValid()) CurrTask->OnPlanDone(this, BhvResult);
		}
	}

	if (CurrTask.IsValid() && CurrTask->IsSatisfied()) CurrTask = NULL;

	OK;
}
//---------------------------------------------------------------------

bool CPropActorBrain::OnUpdateTransform(const Events::CEventBase& Event)
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

bool CPropActorBrain::BeforePhysicsTick(const Events::CEventBase& Event)
{
#ifndef _EDITOR
	MotorSystem.Update(((const Events::CEvent&)Event).Params->Get<float>(CStrID("FrameTime")));
#endif
	OK;
}
//---------------------------------------------------------------------

// We need to react on subframe transform changes to drive physics correctly
bool CPropActorBrain::AfterPhysicsTick(const Events::CEventBase& Event)
{
	//???or access node controller directly and read subframe data?
	Prop::CPropCharacterController* pCC = GetEntity()->GetProperty<Prop::CPropCharacterController>();
	if (!pCC) OK;

	//!!!FIXME! write to the Bullet support:
	// It is strange, but post-tick callback is called before synchronizeMotionStates(), so the body
	// has an outdated transformation here. So we have to access object's world tfm.
	vector3 OldPos = Position;
	quaternion Rot;
	pCC->GetController()->GetBody()->Physics::CPhysicsObj::GetTransform(Position, Rot); //!!!need nonvirtual method "GetWorld/PhysicsTransform"!
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

bool CPropActorBrain::OnAddTask(const Events::CEventBase& Event)
{
	AI::PTask Task = ((const Event::QueueTask&)Event).Task;
	if (Task->IsAvailableTo(this))
	{
		//!!!now task is erased, can queue it instead! bool ClearQueue in event.
		if (CurrTask.IsValid()) CurrTask->Abort(this);
		CurrTask = Task;
		if (SetPlan(Task->BuildPlan())) Task->OnPlanSet(this);
	}

	OK;
}
//---------------------------------------------------------------------

bool CPropActorBrain::OnNavMeshDataChanged(const Events::CEventBase& Event)
{
	//!!!test when actor goes somewhere!
	NavSystem.SetupState();
	SetPlan(NULL);
	RequestGoalUpdate();
	OK;
}
//---------------------------------------------------------------------

bool CPropActorBrain::OnRenderDebug(const Events::CEventBase& Event)
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