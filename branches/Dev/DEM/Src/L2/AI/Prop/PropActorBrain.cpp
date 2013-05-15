#include "PropActorBrain.h"

#include <AI/Prop/PropSmartObject.h>
#include <AI/AIServer.h>
#include <AI/Behaviour/Action.h>
#include <AI/Planning/GoalIdle.h>
#include <AI/Events/QueueTask.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <Game/Mgr/FocusManager.h>	// For dbg rendering
#include <Game/EntityManager.h>
#include <Game/GameServer.h>
#include <Loading/EntityFactory.h>
#include <DB/DBServer.h>

#ifdef __WIN32__
	#undef GetClassName
	#ifdef GetProp
		#undef GetProp
		#undef SetProp
	#endif
#endif

namespace Attr
{
	DeclareAttr(Transform);

	DefineString(ActorDesc);
	DefineFloat(Radius);
	DefineFloat(Height);
};

BEGIN_ATTRS_REGISTRATION(PropActorBrain)
	RegisterString(ActorDesc, ReadOnly);
	RegisterFloatWithDefault(Radius, ReadWrite, 0.3f);
	RegisterFloatWithDefault(Height, ReadWrite, 1.75f);
END_ATTRS_REGISTRATION

namespace Properties
{
__ImplementClass(Properties::CPropActorBrain, 'PRAB', Game::CProperty);
__ImplementPropertyStorage(CPropActorBrain);

static const nString StrPercPrefix("AI::CPerceptor");
static const nString StrSensorPrefix("AI::CSensor");
static const nString StrGoalPrefix("AI::CGoal");

// At very small speeds and physics step sizes body position stops updating because of limited
// float precision. LinearSpeed = 0.0007f, StepSize = 0.01f, Pos + LinearSpeed * StepSize = Pos.
// So body never reaches the desired destination and we must accept arrival at given tolerance.
// The less is this value, the more precise is resulting position, but the more time arrival takes.
// Empirical minimum value is somewhere around 0.0008f.
// This value is measured in game world meters.
// NB: may be square of this value MUST make sense with a single (float) precision too.
const float CPropActorBrain::ArrivalTolerance = 0.009f;

CPropActorBrain::CPropActorBrain(): MemSystem(this), NavSystem(this), MotorSystem(this)
{
}
//---------------------------------------------------------------------

void CPropActorBrain::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	Attrs.Append(Attr::ActorDesc);
	Attrs.Append(Attr::Radius);
	Attrs.Append(Attr::Height);
}
//---------------------------------------------------------------------

void CPropActorBrain::Activate()
{
	Game::CProperty::Activate();

// Blackboard

	const matrix44& Tfm = GetEntity()->Get<matrix44>(Attr::Transform);
	Position = Tfm.Translation();
	LookatDir = -Tfm.AxisZ();

	//!!!update on R/H attrs change!
	Radius = GetEntity()->Get<float>(Attr::Radius);
	Height = GetEntity()->Get<float>(Attr::Height);

	NavStatus = AINav_Done;

	MvmtStatus = AIMvmt_None;
	MvmtType = AIMvmt_Type_Walk;
	SteeringType = AISteer_Type_Seek;
	MinReachDist = 0.f;
	MaxReachDist = 0.f;
	FacingStatus = AIFacing_Done;

// END Blackboard
	
	static const nString DefaultState("Default");

	//???need to cache?
	PParams Desc;
	if (DataSrv->LoadDesc(Desc, nString("actors:") + GetEntity()->Get<nString>(Attr::ActorDesc) + ".prm"))
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
				Perceptors.Append(New);
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
				Sensors.Append(New);
				
				PDataArray Percs = NewDesc->Get<PDataArray>(CStrID("Perceptors"), NULL);
				if (Percs.IsValid())
				{
					CDataArray::iterator ItPercName;
					for (ItPercName = Percs->Begin(); ItPercName != Percs->End(); ItPercName++)
					{
						nString PercClass = StrPercPrefix + ItPercName->GetValue<nString>();
						bool Found = false;

						nArray<PPerceptor>::iterator ItPerc;
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
									 ItPercName->GetValue<nString>().CStr(),
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
				Goals.Append(New);
			}

			if (!HasIdleGoal)
			{
				PGoal New = n_new(CGoalIdle);
				New->Init(NULL);
				Goals.Append(New);
			}
		}

		PDataArray ActionArray;
		if (Desc->Get<PDataArray>(ActionArray, CStrID("Actions")))
		{
			Actions.Reallocate(ActionArray->GetCount(), 0);
			for (int i = 0; i < ActionArray->GetCount(); i++)
			{
				LPCSTR pActionName = ActionArray->At(i).GetValue<nString>().CStr();
				const CActionTpl* pTpl = AISrv->GetPlanner().FindActionTpl(pActionName);
				if (pTpl) Actions.Append(pTpl);
				else n_printf("Warning, AI: action template '%s' is not registered\n", pActionName);
			}
		}

		bool DecMaking = Desc->Get<bool>(CStrID("EnableDecisionMaking"), false);
		Flags.SetTo(AIMind_EnableDecisionMaking, DecMaking);
		Flags.SetTo(AIMind_UpdateGoal, DecMaking);
		
		NavSystem.Init(Desc->Get<PParams>(CStrID("Navigation"), NULL).GetUnsafe());
		MotorSystem.Init(Desc->Get<PParams>(CStrID("Movement"), NULL).GetUnsafe());
	}

	PROP_SUBSCRIBE_PEVENT(OnBeginFrame, CPropActorBrain, OnBeginFrameProc);
	PROP_SUBSCRIBE_PEVENT(OnRenderDebug, CPropActorBrain, OnRenderDebug);
	PROP_SUBSCRIBE_PEVENT(ExposeSI, CPropActorBrain, ExposeSI);
	PROP_SUBSCRIBE_PEVENT(UpdateTransform, CPropActorBrain, OnUpdateTransform);
	PROP_SUBSCRIBE_NEVENT(QueueTask, CPropActorBrain, OnAddTask);
	PROP_SUBSCRIBE_PEVENT(OnNavMeshDataChanged, CPropActorBrain, OnNavMeshDataChanged);
}
//---------------------------------------------------------------------

void CPropActorBrain::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnBeginFrame);
	UNSUBSCRIBE_EVENT(OnRenderDebug);
	UNSUBSCRIBE_EVENT(ExposeSI);
	UNSUBSCRIBE_EVENT(UpdateTransform);
	UNSUBSCRIBE_EVENT(QueueTask);
	UNSUBSCRIBE_EVENT(OnNavMeshDataChanged);

	NavSystem.Term();

	Game::CProperty::Deactivate();
}
//---------------------------------------------------------------------

bool CPropActorBrain::OnBeginFrameProc(const Events::CEventBase& Event)
{
	OnBeginFrame(); //!!!only for possible virtual calls!
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

	for (nArray<PGoal>::iterator ppGoal = Goals.Begin(); ppGoal != Goals.End(); ++ppGoal)
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

		for (nArray<PGoal>::iterator ppGoal = Goals.Begin(); ppGoal != Goals.End(); ++ppGoal)
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

void CPropActorBrain::OnBeginFrame()
{
	float FrameTime = (float)GameSrv->GetFrameTime();

	// Update blackboard
	// ...

#ifndef _EDITOR
	NavSystem.Update(FrameTime);
	MotorSystem.Update();
#endif

	// Update animation
	// AnimSystem->Update();

	//???where to update target system? or goal updates target?

	//!!!update for some time, if not updated all return or allow to process next!
	for (nArray<PSensor>::iterator ppSensor = Sensors.Begin(); ppSensor != Sensors.End(); ++ppSensor)
	{
		//!!!only for external! also need to update internal sensors & actor's state through them
		//???CStimulus -> CExternalStimulus?
		AISrv->GetLevel()->UpdateActorsSense(this, (*ppSensor));
	}

	MemSystem.Update();

	if (CurrPlan.IsValid() && (!CurrPlan->IsValid(this) || CurrPlan->Update(this) != Running))
	{
		SetPlan(NULL);
		//if (CurrTask.IsValid()) CurrTask->OnPlanDone(this, BhvResult);
	}

	if (CurrTask.IsValid() && CurrTask->IsSatisfied()) CurrTask = NULL;

	// Disable this flag for player- or script-controlled actors
	//???mb some goals can abort command (as interrupt behaviours)? so check/clear cmd inside in interruption
//!!!???tmp?!
#ifndef _EDITOR
	if (Flags.Is(AIMind_EnableDecisionMaking) && !CurrTask.IsValid()) UpdateDecisionMaking();
#endif
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

bool CPropActorBrain::OnUpdateTransform(const Events::CEventBase& Event)
{
	const matrix44& Tfm = GetEntity()->Get<matrix44>(Attr::Transform);
	Position = Tfm.Translation();
	LookatDir = -Tfm.AxisZ();
	NavSystem.UpdatePosition();
	OK;
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
	if (FocusMgr->GetInputFocusEntity() == GetEntity())
	{
		MotorSystem.RenderDebug();
	}
	OK;
}
//---------------------------------------------------------------------

} // namespace Properties