#pragma once
#include <Data/Ptr.h>
#include <Physics/TickListener.h>
#include <Math/Vector3.h>

// Character controller suitable for bipedal characters with horizontal, vertical movement
// and facing direction. Implemented as a dynamic rigid body and therefore has proper
// collisions, but can be pushed out of navigation mesh.

namespace Physics
{
typedef Ptr<class CRigidBody> PRigidBody;
class CPhysicsLevel;

enum class ECharacterState
{
	Stand = 0, // Stands idle on feet
	Walk,      // Walks with normal speed
	Run,       // Runs with high speed
	Crouch,    // Walks slowly with low profile to be less noticeable
	Jump,      // Above the ground, falls, controls itself
	Fall       // Above the ground, falls, control is lost
	// Lay (or switch to ragdoll?), Levitate
};

class CCharacterController : public ITickListener
{
protected:

	//!!!Slide down from where can't climb up and don't slide where can climb!
	//Slide along vertical obstacles, don't bounce

	/*
	MaxSpeed[AIMvmt_Type_Walk] = 2.5f;
	MaxSpeed[AIMvmt_Type_Run] = 6.f;
	MaxSpeed[AIMvmt_Type_Crouch] = 1.f;
	MaxAngularSpeed = PI;
	ArriveCoeff = -0.5f / -10.f; // -1/2a = -0.5/a, where a is max brake acceleration and must be negative
	*/

	ECharacterState	_State = ECharacterState::Stand;

	float			_Radius = 0.3f;
	float			_Height = 1.75f;
	float			_Hover = 0.2f;	//???is it MaxClimb itself?
	float			MaxSlopeAngle;		// In radians //???recalc to cos?
	float			MaxClimb; //???recalc to hover?
	//float			MaxJumpImpulse;		// Maximum jump impulse (mass- and direction-independent)
	float			MaxLandingImpulse;	//???speed? Maximum impulse the character can handle when landing. When exceeded, falling starts.
	float			MaxStepDownHeight = 0.2f;	// Maximum height above the ground when character controls itself and doesn't fall
	float			Softness;			// Allowed penetration depth //???!!!recalc to bullet collision margin?!
	float           _Mass = 80.f;

	//???need here? or Bullet can do this? if zero or less, don't use
	float			MaxAcceleration = 0.f;

	bool            _Dirty = true;

	PRigidBody		_Body;

	vector3			ReqLinVel = vector3::Zero;
	float			ReqAngVel = 0.f;

	void CalcDesiredLinearVelocity();
	void CalcDesiredAngularVelocity();
	void AvoidObstacles();

public:

	CCharacterController();
	~CCharacterController();

	void            ApplyChanges();
	void            AttachToLevel(CPhysicsLevel& Level);
	void            RemoveFromLevel();

	virtual void    BeforePhysicsTick(CPhysicsLevel* pLevel, float dt) override;
	virtual void    AfterPhysicsTick(CPhysicsLevel* pLevel, float dt) override;

	void			RequestLinearVelocity(const vector3& Velocity) { ReqLinVel = Velocity; }
	void			RequestAngularVelocity(float Velocity) { ReqAngVel = Velocity; }

	void            SetRadius(float Radius) { if (_Radius != Radius) { _Radius = Radius; _Dirty = true; } }
	void            SetHeight(float Height) { if (_Height != Height) { _Height = Height; _Dirty = true; } }
	void            SetHover(float Hover) { if (_Hover != Hover) { _Hover = Hover; _Dirty = true; } }

	float			GetRadius() const { return _Radius; }
	float			GetHeight() const { return _Height; }
	float			GetHover() const { return _Hover; }
	CRigidBody*		GetBody() const { return _Body.Get(); }
	//vector3		GetLinearVelocity(vector3& Out) const;
	const vector3&	GetRequestedLinearVelocity() const { return ReqLinVel; }
	//float			GetAngularVelocity() const;
	float			GetRequestedAngularVelocity() const { return ReqAngVel; }
	bool			IsMotionRequested() const { return IsLinearMotionRequested() || IsAngularMotionRequested(); }
	bool			IsLinearMotionRequested() const { return ReqLinVel != vector3::Zero; }
	bool			IsAngularMotionRequested() const { return ReqAngVel != 0.f; }

	//!!!collision callbacks/events! fire from global or from here?

	//IsOnTheGround / GetGroundInfo / IsFalling
	//Jump //???Fall if touched the ceiling?
	//???StartCrouch, StopCrouch (if can't, now will remember request), IsCrouching?
};

}
