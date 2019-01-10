#pragma once
#ifndef __DEM_L2_PROP_ACTOR_BRAIN_H__
#define __DEM_L2_PROP_ACTOR_BRAIN_H__

#include <Game/Property.h>

#include <AI/Perception/Sensor.h>
#include <AI/Perception/Perceptor.h>
#include <AI/Planning/Goal.h>
#include <AI/Navigation/NavSystem.h>
#include <AI/Movement/MotorSystem.h>
#include <AI/Memory/MemSystem.h>
#include <Data/Flags.h>
#include <Data/List.h>
#include <Game/Entity.h> // Because too many modules using Brain use also Entity

// Represents AI actor (agent). It is only a part of a whole character, which consists of brain and body.
// Capabilities fo the brain are to process world and self state, perform decision making, planning and
// execute the plan. Being executed, plan sends requests to the body and the game logic.
// Body is responsible for transforming incoming requests to movement and animation.

//!!!needs first update delay!

namespace AI
{
	class CActionTpl;
	typedef Ptr<class CAction> PAction;

	const float Relevance_Absolute = FLT_MAX;

	struct CTask
	{
		//???some int/strid ID to send in event OnTaskDone/Aborted?
		PAction	Plan;
		float	Relevance;
		bool	FailOnInterruption;
		bool	ClearQueueOnFailure;
	};
}

namespace Prop
{
using namespace AI;

class CPropActorBrain: public Game::CProperty
{
	__DeclareClass(CPropActorBrain);
	__DeclarePropertyStorage;

protected:

	CMemSystem					MemSystem;
	CNavSystem					NavSystem;
	CMotorSystem				MotorSystem;
	//TargetSystem //???or in goals?
	//AnimSystem
	//???BhvSystem/DecisionSystem?
	//SoundSystem/SpeechSystem

	CArray<PSensor>				Sensors;
	CArray<PPerceptor>			Perceptors; //???may be store only in sensors? sensors die - perceptors die with no source. refcount.
	CArray<PGoal>				Goals;
	CArray<const CActionTpl*>	ActionTpls;

	PGoal						CurrGoal;	// If NULL, task is processed or actor is completely idle
	Data::CList<CTask>			TaskQueue;	// Queue front is a current task
	PAction						Plan;

// Blackboard //???or private flags are not a part of the blackboard?

	enum
	{
		AIMind_EnableDecisionMaking			= 0x0001,
		AIMind_SelectAction					= 0x0002,
		AIMind_InvalidatePlan				= 0x0004,
		AIMind_Nav_AcceptNearestValidDest	= 0x0008,
		AIMind_Nav_IsLocationValid			= 0x0010
		//AIMind_TaskIsActive
	};

	Data::CFlags				Flags;

// END Blackboard

	virtual bool		InternalActivate();
	virtual void		InternalDeactivate();
	void				EnableSI(class CPropScriptable& Prop);
	void				DisableSI(class CPropScriptable& Prop);
	void				UpdateBehaviour();
	void				SetPlan(PAction NewPlan, CGoal* pPrevGoal, UPTR PrevPlanResult);

	DECLARE_EVENT_HANDLER(OnBeginFrame, OnBeginFrame);
	DECLARE_EVENT_HANDLER(OnRenderDebug, OnRenderDebug);
	DECLARE_EVENT_HANDLER(OnPropActivated, OnPropActivated);
	DECLARE_EVENT_HANDLER(OnPropDeactivating, OnPropDeactivating);
	DECLARE_EVENT_HANDLER(UpdateTransform, OnUpdateTransform);
	DECLARE_EVENT_HANDLER(BeforePhysicsTick, BeforePhysicsTick);
	DECLARE_EVENT_HANDLER(AfterPhysicsTick, AfterPhysicsTick);
	DECLARE_EVENT_HANDLER(OnNavMeshDataChanged, OnNavMeshDataChanged);

public:

	static const float LinearArrivalTolerance;
	static const float AngularArrivalTolerance;

// Blackboard

	// Common
	vector3			Position;
	vector3			LookatDir;
	float			Radius;
	float			Height;

	// Navigation
	ENavState		NavState;
	float			DistanceToNavDest;
	float			NavDestRecoveryTime;	// Time before failing if destination is lost with a chance to be recovered

	// Movement
	EMovementState	MvmtState;
	EMovementType	MvmtType;
	ESteeringType	SteeringType;
	EFacingState	FacingState;

// END Blackboard
	
	CPropActorBrain();
	//virtual ~CPropActorBrain();

	bool				IsActionAvailable(const CActionTpl* pAction) const;
	void				FillWorldState(CWorldState& WSCurr) const;

	void				EnqueueTask(const CTask& Task);
	void				ClearTaskQueue();

	//???rename? redesign logic!
	void				AbortCurrAction(UPTR Result); //???to event, like QueueTask?
	//DequeueTask(PTask or iterator, begin() by default)

	void				RequestBehaviourUpdate() { Flags.Set(AIMind_SelectAction); }

	bool				IsAtPoint(const vector3& Point, float MinDistance = 0.f, float MaxDistance = 0.f) const;
	bool				IsLookingAtDir(const vector3& Direction) const;
	bool				IsNavSystemIdle() const { return !!(NavState & NAV_IDLE); }
	void				SetNavLocationValid(bool Valid) { return Flags.SetTo(AIMind_Nav_IsLocationValid, Valid); }
	bool				IsNavLocationValid() const { return Flags.Is(AIMind_Nav_IsLocationValid); }
	void				SetAcceptNearestValidDestination(bool Accept) { return Flags.SetTo(AIMind_Nav_AcceptNearestValidDest, Accept); }
	bool				DoesAcceptNearestValidDestination() const { return Flags.Is(AIMind_Nav_AcceptNearestValidDest); }

	//???!!!public members instead?!
	CMemSystem&			GetMemSystem() { return MemSystem; }
	const CMemSystem&	GetMemSystem() const { return MemSystem; }
	CNavSystem&			GetNavSystem() { return NavSystem; }
	const CNavSystem&	GetNavSystem() const { return NavSystem; }
	CMotorSystem&		GetMotorSystem() { return MotorSystem; }
	const CMotorSystem&	GetMotorSystem() const { return MotorSystem; }

	const CArray<PSensor>& GetSensors() const { return Sensors; }
};
//---------------------------------------------------------------------

inline bool CPropActorBrain::IsActionAvailable(const CActionTpl* pAction) const
{
	for (CArray<const CActionTpl*>::CIterator ppTpl = ActionTpls.Begin(); ppTpl != ActionTpls.End(); ++ppTpl)
		if (*ppTpl == pAction) OK;
	FAIL;
}
//---------------------------------------------------------------------

inline bool CPropActorBrain::IsAtPoint(const vector3& Point, float MinDistance, float MaxDistance) const
{
	if (MinDistance >= LinearArrivalTolerance) MinDistance -= LinearArrivalTolerance;
	MaxDistance += LinearArrivalTolerance;
	const float SqDistToDest2D = vector3::SqDistance2D(Position, Point);
	return	SqDistToDest2D <= MaxDistance * MaxDistance &&
			SqDistToDest2D >= MinDistance * MinDistance &&
			n_fabs(Position.y - Point.y) < Height;
}
//---------------------------------------------------------------------

inline bool CPropActorBrain::IsLookingAtDir(const vector3& Direction) const
{
	return n_fabs(vector3::Angle2DNorm(LookatDir, Direction)) < AngularArrivalTolerance;
}
//---------------------------------------------------------------------

}

#endif