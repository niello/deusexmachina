#pragma once
#ifndef __DEM_L2_IAO_USE_ACTION_TPL_H__
#define __DEM_L2_IAO_USE_ACTION_TPL_H__

#include <AI/Planning/WorldStateSource.h>
#include <Data/SimpleString.h>

// Template record for smart object action, contains shared properties
// to reduce memory footprint and setup efforts

namespace Data
{
	class CParams;
}

namespace AI
{

class CSmartAction
{
public:

	enum EProgressDrv
	{
		PDrv_None = 0,
		PDrv_Duration,
		PDrv_SO_FSM
	};

	enum
	{
		END_ON_DONE				= 0x01,	// If set, end action when Duration passed
		RESET_ON_ABORT			= 0x02,	// If set, clear progress of unfinished action
		MANUAL_TRANSITION		= 0x04,	// If set, transition progress is updated manually, not by the frame timer
		SEND_PROGRESS_EVENT		= 0x08,	// If set, progress chaged event is sent from an AI action
		FACE_OBJECT				= 0x40,	// If set, actor will face host object before starting the action
		ACTOR_RADIUS_MATTERS	= 0x80	// If set, distance is adjusted by the actor radius
		// - disable after use
	};

	Data::CFlags		Flags;

	// Destination and facing //???or purely algorithmic? remove flags if so!
	float				MinDistance;		// Can make union with navmesh region
	float				MaxDistance;		// < 0 = no distance requirements
	vector3				DestOffset;			// Offset from SO position in local coords
	// - local face direction (angle from forward direction around Y axis)

	// State transition and timing
	float				Duration;
	CStrID				TargetState;
	EProgressDrv		ProgressDriver;

	// Usage restrictions
	int					MaxUserCount;		// How much users at a time can execute this action on the same SO, -1 is no limit

	//???planner worldstate source in validator or separate or in this? data-driven set of key-value pairs
	// AI
	PWorldStateSource	Preconditions;
 
	// Optional(?) scripted functions
	//???or use callbacks with predefined names and inside differ by action ID?
	Data::CSimpleString	ValidateFunc;
	Data::CSimpleString	GetDestinationFunc;
	Data::CSimpleString	GetDurationFunc;
	Data::CSimpleString	UpdateFunc;

	void Init(const Data::CParams& Desc);
	bool IsValid(const AI::CActor* pActor, const CPropSmartObject* pSO) const;
	bool EndOnDone() const { return Flags.Is(CSmartAction::END_ON_DONE); }
	bool ResetOnAbort() const { return Flags.Is(CSmartAction::RESET_ON_ABORT); }
	bool ManualTransitionControl() const { return Flags.Is(CSmartAction::MANUAL_TRANSITION); }
	bool SendProgressEvent() const { return Flags.Is(CSmartAction::SEND_PROGRESS_EVENT); }
	bool FaceObject() const { return Flags.Is(CSmartAction::FACE_OBJECT); }
	bool ActorRadiusMatters() const { return Flags.Is(CSmartAction::ACTOR_RADIUS_MATTERS); }
};

inline bool CSmartAction::IsValid(const AI::CActor* pActor, const CPropSmartObject* pSO) const
{
	//if _TARGET_STATE and SO.FSM.IsInTransition() and SO.FSM.GetTransitionActionID() != _ACTION_ID
	//	fail
		//ActivationValidator.IsValid() ? ActivationValidator->IsValid(pActor, pSO, this) : !!pActor;
		//if (!ConditionFunc.IsValid()) OK;
		//CPropScriptable* pScriptable = pSO->GetEntity()->GetProperty<CPropScriptable>();
		//CScriptObject* pScriptObj = pScriptable ? pScriptable->GetScriptObject() : NULL;
		//CStrID ActorID = pActor ? pActor->GetEntity()->GetUID() : CStrID::Empty;
		//return pScriptObj && pScriptObj->RunFunctionOneArg(ConditionFunc, ActorID) == Success;
	n_error("IMPLEMENT ME!!!");
	FAIL;
}
//---------------------------------------------------------------------

}

#endif