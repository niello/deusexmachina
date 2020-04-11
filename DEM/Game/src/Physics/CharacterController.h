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

	enum class EMovementState : U8
	{
		Requested, // Moving to the requested point or rotating to the requested direction
		Idle,      // Stands idle with the movement request satisfied
		Stuck      // Stands idle with the movement failed
	};

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
	EMovementState  _LinearMovementState = EMovementState::Idle;
	EMovementState  _AngularMovementState = EMovementState::Idle;
	vector3         _RequestedPosition;
	vector3         _NextRequestedPosition;
	vector3         _RequestedLookat; //???or float angle along Y is enough?
	float           _MaxLinearSpeed = 3.f;
	float           _MaxAngularSpeed = PI;
	bool            _NeedDestinationFacing = false; //!!!!!for short step only, maybe get rid of the flag?
	bool            _ObstacleAvoidanceEnabled = false;
	float           _SteeringSmoothness = 0.3f;
	float           _AdditionalArriveDistance = -1.f; // If < 0, no arrive steering requested at all

	float           _ArriveBrakingCoeff = -0.5f / -10.f; // -1/2a = -0.5/a, where a is max brake acceleration, a < 0
	float           _BigTurnThreshold = PI / 3.f;        // Max angle (in rad) actor can turn without stopping linear movement

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

	vector3			_DesiredLinearVelocity = vector3::Zero;
	float			_DesiredAngularVelocity = 0.f;

	float   CalcDistanceToGround(const vector3& Pos) const;
	vector3 CalcDesiredLinearVelocity(const vector3& Pos) const;
	float   CalcDesiredAngularVelocity(float Angle) const;
	void    AvoidObstacles();

public:

	CCharacterController();
	~CCharacterController();

	void            ApplyChanges();
	void            AttachToLevel(CPhysicsLevel& Level);
	void            RemoveFromLevel();

	virtual void    BeforePhysicsTick(CPhysicsLevel* pLevel, float dt) override;
	virtual void    AfterPhysicsTick(CPhysicsLevel* pLevel, float dt) override;

	//???separate SetSpeed? to make internal requests like facing movement direction!
	void            RequestMovement(const vector3& Dest) { RequestMovement(Dest, Dest); }
	void            RequestMovement(const vector3& Dest, const vector3& NextDest);
	void            RequestFacing(const vector3& Direction);
	void			RequestLinearVelocity(const vector3& Velocity) { _DesiredLinearVelocity = Velocity; }
	void			RequestAngularVelocity(float Velocity) { _DesiredAngularVelocity = Velocity; }
	void            ResetMovement() { _LinearMovementState = EMovementState::Idle; _DesiredLinearVelocity = vector3::Zero; }
	void            ResetFacing() { _AngularMovementState = EMovementState::Idle; _DesiredAngularVelocity = 0.f; }
	void            SetMaxLinearSpeed(float Speed) { _MaxLinearSpeed = std::max(0.f, Speed); }
	void            SetMaxAngularSpeed(float Speed) { _MaxAngularSpeed = std::max(0.f, Speed); }
	void            SetArrivalParams(bool Enable, float AdditionalDistance = 0.f);

	void            SetRadius(float Radius) { if (_Radius != Radius) { _Radius = Radius; _Dirty = true; } }
	void            SetHeight(float Height) { if (_Height != Height) { _Height = Height; _Dirty = true; } }
	void            SetHover(float Hover) { if (_Hover != Hover) { _Hover = Hover; _Dirty = true; } }

	float			GetRadius() const { return _Radius; }
	float			GetHeight() const { return _Height; }
	float			GetHover() const { return _Hover; }
	CRigidBody*		GetBody() const { return _Body.Get(); }
	vector3         GetLinearVelocity() const;
	float           GetAngularVelocity() const;
	const vector3&	GetRequestedLinearVelocity() const { return _DesiredLinearVelocity; }
	float			GetRequestedAngularVelocity() const { return _DesiredAngularVelocity; }
	bool			IsMotionRequested() const { return IsLinearMotionRequested() || IsAngularMotionRequested(); }
	bool			IsLinearMotionRequested() const { return _DesiredLinearVelocity != vector3::Zero; }
	bool			IsAngularMotionRequested() const { return _DesiredAngularVelocity != 0.f; }
	bool            IsOnTheGround() const { return _State != ECharacterState::Jump && _State != ECharacterState::Fall; } // Levitate too!

	//Jump //???Fall if touched the ceiling?
	//???StartCrouch, StopCrouch (if can't, now will remember request), IsCrouching?
};

}
