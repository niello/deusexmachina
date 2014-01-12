#pragma once
#ifndef __DEM_L2_AI_MOTOR_SYSTEM_H__
#define __DEM_L2_AI_MOTOR_SYSTEM_H__

#include <StdDEM.h>
#include <AI/ActorFwd.h>
#include <Core/Ptr.h>
#include <mathlib/vector.h>

// Motor system is responsible for steering, facing and other position/orientation
// changes of the actor. This system uses physics interface to perform actual movement.

namespace Data
{
	typedef Ptr<class CParams> PParams;
};

namespace AI
{
class CMemFactObstacle;

enum EMovementState
{
	AIMvmt_Done,		// Steering destination is reached
	AIMvmt_Failed,		// No movement is currently performed
	AIMvmt_DestSet,		// There is a valid destination for steering
	AIMvmt_Stuck		// Actor got stuck during the movement
};

enum EFacingState
{
	AIFacing_Done,
	AIFacing_Failed,
	AIFacing_DirSet
};

enum EMovementType
{
	AIMvmt_Type_None,	// Actor is standing still
	AIMvmt_Type_Walk,
	AIMvmt_Type_Run,
	AIMvmt_Type_Crouch,

	AIMvmt_Type_Count
};

enum ESteeringType
{
	AISteer_Type_Seek,
	AISteer_Type_Arrive,

	AISteer_Type_Count
};

class CMotorSystem
{
protected:

	CActor*				pActor;

	//???Here or on the BB?
	float				MaxSpeed[AIMvmt_Type_Count]; // If <= 0.f, not available to this actor
	float				MaxAngularSpeed;
	float				ArriveCoeff;			// -1 / 2 * a, where a is a maximum braking acceleration, a < 0

	//!!!???BB?!
	float				SqShortStepThreshold;	// Max amount of movement actor can move without facing a destination 
	float				BigTurnThreshold;		// Max angle (in rad) actor can turn without stopping to move

	vector3				DestPoint;
	vector3				NextDestPoint;

	vector3				FaceDir;

	//???!!!BB?!
	bool				FaceDest;
	bool				AvoidObstacles; //???!!!to actor BB flags?!
	bool				SmoothSteering; //???!!!to actor BB flags?!
	bool				AdaptiveVelocitySampling;
	float				AvoidanceMargin;
	CMemFactObstacle*	pLastClosestObstacle;
	vector2				LastClosestObstaclePos;
	vector2				LastAvoidDir;

	//state
	//dest
	//rotation info (time etc)
	//???!!!to BB!? curr orientation // Millington: (radians CCW from +z) //???or-z in my case?

	//last dist to dest
	//stuck time //???BB?

	bool IsStuck();

public:

	CMotorSystem(CActor* Actor);

	void	Init(const Data::CParams* Params);
	void	Update(float FrameTime);
	void	UpdatePosition();
	void	ResetMovement(bool Success);
	void	ResetRotation(bool Success);
	void	RenderDebug();

	void	SetDest(const vector3& Dest);
	void	SetNextDest(const vector3& NextDest) { NextDestPoint = NextDest; }
	void	SetFaceDirection(const vector3& Dir);

	float	GetMaxSpeed() const;
};

inline CMotorSystem::CMotorSystem(CActor* Actor):
	pActor(Actor),
	SqShortStepThreshold(1.f * 1.f),
	BigTurnThreshold(PI / 3.f), //0.5f * PI),
	AvoidObstacles(true),
	AdaptiveVelocitySampling(true),
	AvoidanceMargin(0.1f),
	pLastClosestObstacle(NULL)
{
}
//---------------------------------------------------------------------

}

#endif