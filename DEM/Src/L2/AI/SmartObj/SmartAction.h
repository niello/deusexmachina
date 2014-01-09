#pragma once
#ifndef __DEM_L2_IAO_USE_ACTION_TPL_H__
#define __DEM_L2_IAO_USE_ACTION_TPL_H__

#include <AI/Planning/WorldStateSource.h>
#include <Data/SimpleString.h>

// Smart action holds information for an interaction with a smart object

namespace Data
{
	class CParams;
}

namespace AI
{

class CSmartAction
{
protected:

	Data::CFlags Flags;

public:

	enum EProgressDrv
	{
		PDrv_None = 0,
		PDrv_Duration,
		PDrv_SO_FSM
	};

	enum
	{
		END_ON_DONE				= 0x0001,	// If set, end action when Duration passed
		RESET_ON_ABORT			= 0x0002,	// If set, clear progress of unfinished action
		MANUAL_TRANSITION		= 0x0004,	// If set, transition progress is updated manually, not by the frame timer
		SEND_PROGRESS_EVENT		= 0x0008,	// If set, progress chaged event is sent from an AI action
		//SYNC_ACTOR_ANIMATION	= 0x0010,	// If set, actor animation duration is synchronized with an action duration
		FACE_OBJECT				= 0x0040,	// If set, actor will face host object before starting the action
		ACTOR_RADIUS_MATTERS	= 0x0080	// If set, distance is adjusted by the actor radius
	};

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

	void Init(const Data::CParams& Desc);
	bool EndOnDone() const { return Flags.Is(CSmartAction::END_ON_DONE); }
	bool ResetOnAbort() const { return Flags.Is(CSmartAction::RESET_ON_ABORT); }
	bool ManualTransitionControl() const { return Flags.Is(CSmartAction::MANUAL_TRANSITION); }
	bool SendProgressEvent() const { return Flags.Is(CSmartAction::SEND_PROGRESS_EVENT); }
	bool FaceObject() const { return Flags.Is(CSmartAction::FACE_OBJECT); }
	bool ActorRadiusMatters() const { return Flags.Is(CSmartAction::ACTOR_RADIUS_MATTERS); }
};

}

#endif