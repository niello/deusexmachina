#include "CharacterController.h"
#include <Physics/BulletConv.h>
#include <Physics/PhysicsLevel.h>
#include <Physics/RigidBody.h>
#include <Physics/CollisionShape.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>

namespace Physics
{
CCharacterController::CCharacterController() = default;
CCharacterController::~CCharacterController() = default;

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
	if (pLevel) _Body->AttachToLevel(*pLevel);

	_Dirty = false;
}
//---------------------------------------------------------------------

void CCharacterController::Update(float dt)
{
	// Calculate the distance to the nearest object under feet
	float DistanceToGround = std::numeric_limits<float>().max();
	{
		constexpr float GroundProbeLength = 0.5f;

		//!!!FIXME! write to the Bullet support:
		// It is strange, but post-tick callback is called before synchronizeMotionStates(), so the body
		// has an outdated transformation here. So we have to access object's world tfm.
		const auto& BodyTfm = _Body->GetBtBody()->getWorldTransform();
		const vector3 Pos = BtVectorToVector(BodyTfm.getOrigin());
		vector3 Start = Pos;
		vector3 End = Pos;
		Start.y += _Height;
		End.y -= (MaxStepDownHeight + GroundProbeLength); // Falling state detection

		// FIXME: improve passing collision flags through interfaces!
		vector3 ContactPos;
		if (_Body->GetLevel()->GetClosestRayContact(Start, End,
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
			_Body->SetActive(true);
		}
		else if (VerticalImpulse > 0.f)
		{
			// send event OnEnd[state] (//???through callback?)
			// send event OnStartJumping (//???through callback?)
			_Body->SetActive(true);
		}
		//???else if VerticalImpulse == 0.f levitate?
	}

//!!!DBG TMP!
return;

	if (_State == Char_Standing)
	{
		// We want a precise control over the movement, so deny freezing on low speed, if movement is requested
		_Body->SetActive(true, IsMotionRequested());

		// No angular acceleration limit, set directly
		_Body->GetBtBody()->setAngularVelocity(btVector3(0.f, ReqAngVel, 0.f));

		const float InvTickTime = 1.f / _Body->GetLevel()->GetStepTime();

		// TODO: remove if all is OK. Just to verify consistency with old logic.
		//const float InvTickTime = 1.f / dt;
		//n_assert_dbg(std::abs(dt - _Body->GetLevel()->GetStepTime()) < 0.000001f);

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
		_Body->SetActive(true, true);
	}
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
