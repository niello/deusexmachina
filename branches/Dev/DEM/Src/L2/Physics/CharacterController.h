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

class CCharacterController: public Core::CRefCounted
{
protected:

	float		Radius;
	float		Height;
	float		Hover;
	float		MaxSlopeAngle;	// In radians
	float		MaxClimb; //???recalc to hover?
	float		MaxJump; //???!!!recalc to max jump impulse?
	float		Softness;		// Allowed penetration depth //???!!!recalc to bullet collision margin?!

	PRigidBody	Body;

public:

	bool Init(const Data::CParams& Desc);
	void Term();
	//AttachToLevel()
	//RemoveFromLevel()

	void RequestLinearVelocity(const vector3& Velocity);
	void RequestAngularVelocity(float Velocity);
};

typedef Ptr<CCharacterController> PCharacterController;

}

#endif
