#pragma once
#ifndef __DEM_L2_IAO_USE_ACTION_TPL_H__
#define __DEM_L2_IAO_USE_ACTION_TPL_H__

#include <Data/Ptr.h>
#include <Data/Flags.h>
#include <Data/StringID.h>
#include <Math/vector3.h>

// Smart action holds information for an interaction with a smart object

namespace Data
{
	class CParams;
}

namespace Scripting
{
	typedef Ptr<class CScriptObject> PScriptObject;
}

namespace AI
{
typedef Ptr<class CWorldStateSource> PWorldStateSource;

class CSmartAction
{
protected:

	CStrID						ID;
	Data::CFlags				Flags;
	Scripting::PScriptObject	ScriptObj;
	float						Duration;

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

	// Destination and facing
	float				MinRange;
	float				MaxRange;
	vector3				PosOffset;	// Local //???is used?
	//float				FaceOffset;	// Local around Y axis

	// State transition and timing
	CStrID				TargetState;
	EProgressDrv		ProgressDriver;

	// Usage restrictions
	int					MaxUserCount;		// How much users at a time can execute this action on the same SO, -1 is no limit

	// AI
	PWorldStateSource	Preconditions;

	CSmartAction();
	~CSmartAction();

	void	Init(CStrID ActionID, const Data::CParams& Desc);

	bool	IsValid(CStrID ActorID, CStrID SOID) const;
	float	GetDuration(CStrID ActorID, CStrID SOID) const;
	UPTR	Update(CStrID ActorID, CStrID SOID) const;

	CStrID	GetID() const { return ID; }
	bool	EndOnDone() const { return Flags.Is(CSmartAction::END_ON_DONE); }
	bool	ResetOnAbort() const { return Flags.Is(CSmartAction::RESET_ON_ABORT); }
	bool	ManualTransitionControl() const { return Flags.Is(CSmartAction::MANUAL_TRANSITION); }
	bool	SendProgressEvent() const { return Flags.Is(CSmartAction::SEND_PROGRESS_EVENT); }
	bool	FaceObject() const { return Flags.Is(CSmartAction::FACE_OBJECT); }
	bool	ActorRadiusMatters() const { return Flags.Is(CSmartAction::ACTOR_RADIUS_MATTERS); }
};

}

#endif