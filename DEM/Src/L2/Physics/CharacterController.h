#pragma once
#ifndef __DEM_L2_CHARACTER_CTLR_H__
#define __DEM_L2_CHARACTER_CTLR_H__

#include <Core/RefCounted.h>

// Character controller is used to drive characters. It gets desired velocities and other commands
// as input and calculates final transform, taking different character properties into account.
// This is a dynamic implementation, that uses a rigid body and works from physics to scene.
// Maybe later we will implement a kinematic one too. Kinematic controller is more controllable
// and is bound to the navmesh, but all the collision detection and response must be processed manually.

namespace Data
{
	class CParams;
}

namespace Physics
{
typedef Ptr<class CRigidBody> PRigidBody;
class CPhysicsWorld;

class CCharacterController: public Core::CRefCounted
{
protected:

	//!!!Slide down from where can't climb up and don't slide where can climb!
	//Slide along vertical obstacles, don't bounce

	float		Radius;
	float		Height;
	float		Hover;	//???is it MaxClimb itself?
	float		MaxSlopeAngle;	// In radians //???recalc to cos?
	float		MaxClimb; //???recalc to hover?
	float		MaxJump; //???!!!recalc to max jump impulse?
	float		Softness;		// Allowed penetration depth //???!!!recalc to bullet collision margin?!

	//???need here? or Bullet can do this? if zero or less, don't use
	float		MaxAcceleration;

	PRigidBody	Body;

	vector3		ReqLinVel;
	float		ReqAngVel;

public:

	~CCharacterController() { Term(); }

	bool			Init(const Data::CParams& Desc);
	void			Term();
	bool			AttachToLevel(CPhysicsWorld& World);
	void			RemoveFromLevel();
	bool			IsAttachedToLevel() const;

	void			Update();

	void			RequestLinearVelocity(const vector3& Velocity) { ReqLinVel = Velocity; }
	void			RequestAngularVelocity(float Velocity) { ReqAngVel = Velocity; }

	float			GetRadius() const { return Radius; }
	float			GetHeight() const { return Height; }
	float			GetHover() const { return Hover; }
	CRigidBody*		GetBody() const { return Body.GetUnsafe(); }
	bool			GetLinearVelocity(vector3& Out) const;
	const vector3&	GetRequestedLinearVelocity() const { return ReqLinVel; }
	float			GetAngularVelocity() const;
	float			GetRequestedAngularVelocity() const { return ReqAngVel; }
	bool			IsMotionRequested() const { return IsLinearMotionRequested() || IsAngularMotionRequested(); }
	bool			IsLinearMotionRequested() const { return ReqLinVel != vector3::Zero; }
	bool			IsAngularMotionRequested() const { return ReqAngVel != 0.f; }

	//!!!collision callbacks/events! fire from global or from here?

	//IsOnTheGround / GetGroundInfo / IsFalling
	//Jump //???Fall if touched the ceiling?
	//???StartCrouch, StopCrouch (if can't, now will remember request), IsCrouching?
};

typedef Ptr<CCharacterController> PCharacterController;

}

#endif
