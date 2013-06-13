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
	typedef Ptr<class CTask> PTask;
}

namespace Prop
{
using namespace AI;

class CPropActorBrain: public Game::CProperty
{
	__DeclareClass(CPropActorBrain);
	__DeclarePropertyStorage;

protected:

	static const float			ArrivalTolerance;

	CMemSystem					MemSystem;
	CNavSystem					NavSystem;
	CMotorSystem				MotorSystem;
	//TargetSystem //???or in goals?
	//AnimSystem
	//???BhvSystem/DecisionSystem?
	//SoundSystem/SpeechSystem

	nArray<PSensor>				Sensors;
	nArray<PPerceptor>			Perceptors; //???may be store only in sensors? sensors die - perceptors die with no source. refcount.
	nArray<PGoal>				Goals;
	nArray<const CActionTpl*>	Actions;

	PGoal						CurrGoal;
	PAction						CurrPlan;
	PTask						CurrTask; //!!!can store task queue!

// Blackboard //???or private flags are not a part of the blackboard?

	enum
	{
		AIMind_EnableDecisionMaking		= 0x0001,
		AIMind_UpdateGoal				= 0x0002,
		AIMind_InvalidatePlan			= 0x0004
		//AIMind_TaskIsActive
	};

	Data::CFlags				Flags;

// END Blackboard

	virtual bool		InternalActivate();
	virtual void		InternalDeactivate();
	void				UpdateDecisionMaking();

	DECLARE_EVENT_HANDLER(OnBeginFrame, OnBeginFrameProc);
	DECLARE_EVENT_HANDLER(OnRenderDebug, OnRenderDebug);
	DECLARE_EVENT_HANDLER(ExposeSI, ExposeSI);
	DECLARE_EVENT_HANDLER(UpdateTransform, OnUpdateTransform);
	DECLARE_EVENT_HANDLER(QueueTask, OnAddTask);
	DECLARE_EVENT_HANDLER(OnNavMeshDataChanged, OnNavMeshDataChanged);

public:

// Blackboard

	// Common
	vector3			Position;
	vector3			LookatDir;
	float			Radius;
	float			Height;

	// Navigation
	ENavStatus		NavStatus;
	float			DistanceToNavDest;

	// Movement
	EMovementStatus	MvmtStatus;
	EMovementType	MvmtType;
	ESteeringType	SteeringType;
	float			MinReachDist;
	float			MaxReachDist;
	EFacingStatus	FacingStatus;

// END Blackboard
	
	CPropActorBrain();
	//virtual ~CPropActorBrain();

	virtual void	OnBeginFrame();

	bool			IsActionAvailable(const CActionTpl* pAction) const;
	void			FillWorldState(CWorldState& WSCurr) const;

	// Mainly is an interface for commands
	bool			SetPlan(CAction* pNewPlan);

	void			RequestGoalUpdate() { Flags.Set(AIMind_UpdateGoal); }

	bool			IsAtPoint(const vector3& Point, bool RespectReachDistances) const;

	CMemSystem&		GetMemSystem() { return MemSystem; }
	CNavSystem&		GetNavSystem() { return NavSystem; }
	CMotorSystem&	GetMotorSystem() { return MotorSystem; }

	const nArray<PSensor>& GetSensors() const { return Sensors; }
};
//---------------------------------------------------------------------

inline bool CPropActorBrain::IsActionAvailable(const CActionTpl* pAction) const
{
	for (nArray<const CActionTpl*>::CIterator ppTpl = Actions.Begin(); ppTpl != Actions.End(); ppTpl++)
		if (*ppTpl == pAction) OK;
	FAIL;
}
//---------------------------------------------------------------------

inline bool CPropActorBrain::IsAtPoint(const vector3& Point, bool RespectReachDistances) const
{
	const float MinReach = (RespectReachDistances ? MinReachDist : 0.f) - ArrivalTolerance;
	const float MaxReach = (RespectReachDistances ? MaxReachDist : 0.f) + ArrivalTolerance;
	const float OffsetX = Position.x - Point.x;
	const float OffsetZ = Position.z - Point.z;
	const float SqDistToDest2D = OffsetX * OffsetX + OffsetZ * OffsetZ;
	return (SqDistToDest2D <= MaxReach * MaxReach &&
			(MinReach <= 0.f || SqDistToDest2D >= MinReach * MinReach) &&
			n_fabs(Position.y - Point.y) < Height);
}
//---------------------------------------------------------------------

}

#endif