#include "CharacterController.h"
#include <Physics/PhysicsLevel.h>
#include <Physics/RigidBody.h>
#include <Physics/CollisionShape.h>
#include <Physics/BulletConv.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>

namespace Physics
{

CCharacterController::CCharacterController() = default;

CCharacterController::~CCharacterController()
{
	if (_Body) RemoveFromLevel();
}
//---------------------------------------------------------------------

void CCharacterController::ApplyChanges()
{
	if (!_Dirty) return;

	Physics::CPhysicsLevel* pLevel = nullptr;
	Scene::CSceneNode* pNode = nullptr;
	matrix44 Tfm;
	if (_Body)
	{
		pLevel = _Body->GetLevel();
		pNode = _Body->GetControlledNode();
		_Body->GetTransform(Tfm);

		RemoveFromLevel();
	}

	// FIXME PHYSICS - where to set? In component (externally)?
	CStrID CollisionGroupID("Character");
	CStrID CollisionMaskID("All");

	const float CapsuleHeight = _Height - _Radius - _Radius - _Hover;
	n_assert (CapsuleHeight > 0.f);
	const vector3 Offset(0.f, (_Hover + _Height) * 0.5f, 0.f);
	PCollisionShape Shape = CCollisionShape::CreateCapsuleY(_Radius, CapsuleHeight, Offset);

	_Body = n_new(Physics::CRigidBody(_Mass, *Shape, CollisionGroupID, CollisionMaskID, Tfm));
	_Body->GetBtBody()->setAngularFactor(btVector3(0.f, 1.f, 0.f));

	if (pNode) _Body->SetControlledNode(pNode);
	if (pLevel) AttachToLevel(*pLevel);

	_Dirty = false;
}
//---------------------------------------------------------------------

void CCharacterController::AttachToLevel(CPhysicsLevel& Level)
{
	if (_Body->GetLevel()) RemoveFromLevel();
	_Body->AttachToLevel(Level);
	Level.RegisterTickListener(this);
}
//---------------------------------------------------------------------

void CCharacterController::RemoveFromLevel()
{
	if (auto pLevel = _Body->GetLevel())
	{
		pLevel->UnregisterTickListener(this);
		_Body->RemoveFromLevel();
	}
}
//---------------------------------------------------------------------

void CCharacterController::BeforePhysicsTick(CPhysicsLevel* pLevel, float dt)
{
	n_assert_dbg(_Body && _Body->GetLevel() == pLevel);

	// Calculate the distance to the nearest object under feet
	float DistanceToGround = std::numeric_limits<float>().max();
	{
		constexpr float GroundProbeLength = 0.5f;

		// Post-tick callback is called just before synchronizeMotionStates(), so the body has an outdated motion state
		// transformation here. Access raw tfm. synchronizeSingleMotionState() could make sense too with dirty flag optimization.
		const auto& BodyTfm = _Body->GetBtBody()->getWorldTransform();
		const vector3 Pos = BtVectorToVector(BodyTfm.getOrigin()) - _Body->GetCollisionShape()->GetOffset();
		vector3 Start = Pos;
		vector3 End = Pos;
		Start.y += _Height;
		End.y -= (MaxStepDownHeight + GroundProbeLength); // Falling state detection

		// FIXME: improve passing collision flags through interfaces!
		vector3 ContactPos;
		if (pLevel->GetClosestRayContact(Start, End,
			_Body->GetBtBody()->getBroadphaseProxy()->m_collisionFilterGroup,
			_Body->GetBtBody()->getBroadphaseProxy()->m_collisionFilterMask,
			&ContactPos, nullptr, _Body.Get()))
		{
			DistanceToGround = Pos.y - ContactPos.y;
		}
	}

	if (DistanceToGround <= 0.f && _State != Char_Standing)
	{
		if (_State == Char_Jumping)
		{
			// send event OnEndJumping (//???through callback?)
			// reset XZ velocity
		}
		else if (_State == Char_Falling)
		{
			// send event OnEndFalling (//???through callback?)
			//???how to prevent character from taking control over itself until it is recovered from a falling?
		}

		// send event OnStartStanding (//???through callback?)
		_State = Char_Standing;
		//???add Char_Laying uncontrolled state after a fall or when on the ground? and then recover
		//can even change collision shape for this state
	}
	else if (DistanceToGround > MaxStepDownHeight && _State == Char_Standing)
	{
		//???controll whole speed or only a vertical component? now the second

		const btVector3& CurrLVel = _Body->GetBtBody()->getLinearVelocity();
		float VerticalImpulse = -_Body->GetMass() * CurrLVel.y(); // Inverted to be positive when directed downwards
		if (VerticalImpulse > MaxLandingImpulse)
		{
			// send event OnEnd[state] (//???through callback?)
			// send event OnStartFalling (//???through callback?)
			_State = Char_Falling;
			_Body->SetActive(true);
		}
		else if (VerticalImpulse > 0.f)
		{
			// send event OnEnd[state] (//???through callback?)
			// send event OnStartJumping (//???through callback?)
			_State = Char_Jumping;
			_Body->SetActive(true);
		}
		//???else if VerticalImpulse == 0.f levitate?
	}

	if (_State == Char_Standing)
	{
		// We want a precise control over the movement, so deny freezing on low speed
		// when movement is requested. When idle, allow to deactivate eventually.
		if (IsMotionRequested())
			_Body->SetActive(true, true);
		else if (_Body->IsAlwaysActive())
			_Body->SetActive(true, false);

		// No angular acceleration limit, set directly
		_Body->GetBtBody()->setAngularVelocity(btVector3(0.f, ReqAngVel, 0.f));

		// TODO: remove assert if all is OK. Just to verify consistency with old logic.
		const float InvTickTime = 1.f / dt;
		n_assert_dbg(std::abs(dt - pLevel->GetStepTime()) < 0.000001f);

		//???what to do with requested y? perform auto climbing/jumping or deny and wait for an explicit command?
		btVector3 ReqLVel = btVector3(ReqLinVel.x, 0.f, ReqLinVel.z);
		if (MaxAcceleration > 0.f)
		{
			const btVector3& CurrLVel = _Body->GetBtBody()->getLinearVelocity();
			btVector3 ReqLVelChange(ReqLVel.x() - CurrLVel.x(), 0.f, ReqLVel.z() - CurrLVel.z());
			if (ReqLVelChange.x() != 0.f || ReqLVelChange.z() != 0.f)
			{
				btVector3 ReqAccel = ReqLVelChange * InvTickTime;
				btScalar AccelMagSq = ReqAccel.length2();
				if (AccelMagSq > MaxAcceleration * MaxAcceleration)
					ReqAccel *= (MaxAcceleration / n_sqrt(AccelMagSq));
				_Body->GetBtBody()->applyCentralForce(ReqAccel * _Body->GetMass());
			}
			else _Body->GetBtBody()->clearForces(); //???really clear all forces?
		}
		else _Body->GetBtBody()->setLinearVelocity(ReqLVel);

		// Compensate gravity by a normal force N, as we are standing on the ground
		_Body->GetBtBody()->applyCentralForce(-_Body->GetBtBody()->getGravity() * _Body->GetMass());
		//???!!!compensate ALL -y force? no ground penetration may happen
		//excess force/impulse (force * tick) can be converted into damage or smth!

		// We want to compensate our DistanceToGround in a single simulation step
		const float ReqVerticalVel = -DistanceToGround * InvTickTime;
		_Body->GetBtBody()->applyCentralImpulse(btVector3(0.f, ReqVerticalVel * _Body->GetMass(), 0.f));
	}
	else
	{
		//???need? gravity should keep the body active
		//if levitating, disable gravity
		//on levitation end, make body active
		//_Body->SetActive(true, true);
	}
}
//---------------------------------------------------------------------

void CCharacterController::AfterPhysicsTick(CPhysicsLevel* pLevel, float dt)
{
}
//---------------------------------------------------------------------

/*
bool CCharacterController::GetLinearVelocity(vector3& Out) const
{
	// FIXME PHYSICS
//	Out = BtVectorToVector(Body->GetBtBody()->getLinearVelocity());
	OK;
}
//---------------------------------------------------------------------

float CCharacterController::GetAngularVelocity() const
{
	// FIXME PHYSICS
//	return _Body->GetBtBody()->getAngularVelocity().y();
	return 0.f;
}
//---------------------------------------------------------------------
*/

}
