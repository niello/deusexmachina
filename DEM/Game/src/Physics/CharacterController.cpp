#include "CharacterController.h"

#include <Physics/BulletConv.h>
#include <Physics/PhysicsLevel.h>
#include <Physics/RigidBody.h>
#include <Physics/ClosestNotMeRayResultCallback.h>
#include <Data/Params.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>

namespace Physics
{
//FACTORY_CLASS_IMPL(Physics::CCharacterController, 'CCTL', Core::CObject);

bool CCharacterController::Init(const Data::CParams& Desc)
{
	State = Char_Standing;

	Radius = Desc.Get<float>(CStrID("Radius"), 0.3f);
	Height = Desc.Get<float>(CStrID("Height"), 1.75f);
	Hover = Desc.Get<float>(CStrID("Hover"), 0.2f);
	MaxAcceleration = Desc.Get<float>(CStrID("MaxAcceleration"), 0.f);
	MaxStepDownHeight = Desc.Get<float>(CStrID("MaxStepDownHeight"), 0.2f);
	n_assert_dbg(MaxStepDownHeight >= 0.f);
	float Mass = Desc.Get<float>(CStrID("Mass"), 80.f);

	float CapsuleHeight = Height - Radius - Radius - Hover;
	n_assert (CapsuleHeight > 0.f);

	vector3 Offset(0.f, (Hover + Height) * 0.5f, 0.f);

	PCollisionShape Shape = CCollisionShape::CreateCapsuleY(Offset, Radius, CapsuleHeight);

	const U16 MaskCharacter = -1;// PhysicsSrv->CollisionGroups.GetMask("Character");
	const U16 MaskAll = -1; // PhysicsSrv->CollisionGroups.GetMask("All");

	Body = n_new(CRigidBody);
	Body->Init(*Shape, Mass, MaskCharacter, MaskAll, Offset);
	Body->GetBtBody()->setAngularFactor(btVector3(0.f, 1.f, 0.f));

	ReqLinVel = vector3::Zero;
	ReqAngVel = 0.f;

	OK;
}
//---------------------------------------------------------------------

void CCharacterController::Term()
{
	RemoveFromLevel();
	Body = nullptr;
}
//---------------------------------------------------------------------

bool CCharacterController::AttachToLevel(CPhysicsLevel& World)
{
	return Body.IsValidPtr() && Body->AttachToLevel(World);
}
//---------------------------------------------------------------------

void CCharacterController::RemoveFromLevel()
{
	if (Body.IsValidPtr()) Body->RemoveFromLevel();
}
//---------------------------------------------------------------------

bool CCharacterController::IsAttachedToLevel() const
{
	return Body.IsValidPtr() && Body->IsAttachedToLevel();
}
//---------------------------------------------------------------------

void CCharacterController::Update()
{
	if (!IsAttachedToLevel()) return;

	//!!!FIXME! write to the Bullet support:
	// It is strange, but post-tick callback is called before synchronizeMotionStates(), so the body
	// has an outdated transformation here. So we have to access object's world tfm.
	vector3 Pos;
	quaternion Rot;
	Body->CPhysicsObject::GetTransform(Pos, Rot); //!!!need nonvirtual method "GetWorld/PhysicsTransform"!

	const float GroundProbeLength = 0.5f;

	btVector3 BtStart = VectorToBtVector(Pos);
	btVector3 BtEnd = BtStart;
	BtStart.m_floats[1] += Height;
	BtEnd.m_floats[1] -= (MaxStepDownHeight + GroundProbeLength); // Falling state detection
	CClosestNotMeRayResultCallback RayCB(BtStart, BtEnd, Body->GetBtObject());
	Body->GetWorld()->GetBtWorld()->rayTest(BtStart, BtEnd, RayCB);

	float DistanceToGround = RayCB.hasHit() ? Pos.y - RayCB.m_hitPointWorld.y() : FLT_MAX;

	if (DistanceToGround <= 0.f && State != Char_Standing)
	{
		if (State == Char_Jumping)
		{
			// send event OnEndJumping (//???through callback?)
			// reset XZ velocity
		}
		else if (State == Char_Falling)
		{
			// send event OnEndFalling (//???through callback?)
			//???how to prevent character from taking control over itself until it is recovered from a falling?
		}

		// send event OnStartStanding (//???through callback?)
		State = Char_Standing;
		//???add Char_Laying uncontrolled state after a fall or when on the ground? and then recover
		//can even change collision shape for this state
	}
	else if (DistanceToGround > MaxStepDownHeight && State == Char_Standing)
	{
		//???controll whole speed or only a vertical component? now the second

		vector3 LinVel;
		GetLinearVelocity(LinVel);
		float VerticalImpulse = -Body->GetMass() * LinVel.y; // Inverted to be positive when directed downwards
		if (VerticalImpulse > MaxLandingImpulse)
		{
			// send event OnEnd[state] (//???through callback?)
			// send event OnStartFalling (//???through callback?)
			Body->MakeActive();
		}
		else if (VerticalImpulse > 0.f)
		{
			// send event OnEnd[state] (//???through callback?)
			// send event OnStartJumping (//???through callback?)
			Body->MakeActive();
		}
		//???else if VerticalImpulse == 0.f levitate?
	}

	if (State == Char_Standing)
	{
		// We want a precise control over the movement, so deny freezing on low speed, if movement is requested
		bool AlwaysActive = Body->IsAlwaysActive();
		bool HasReq = IsMotionRequested();
		if (!AlwaysActive && HasReq) Body->MakeAlwaysActive();
		else if (AlwaysActive && !HasReq) Body->MakeActive();

		// No angular acceleration limit, set directly
		Body->GetBtBody()->setAngularVelocity(btVector3(0.f, ReqAngVel, 0.f));

		const float InvTickTime = 1.f / Body->GetWorld()->GetStepTime();

		//???what to do with requested y? perform auto climbing/jumping or deny and wait for an explicit command?
		btVector3 ReqLVel = btVector3(ReqLinVel.x, 0.f, ReqLinVel.z);
		if (MaxAcceleration > 0.f)
		{
			const btVector3& CurrLVel = Body->GetBtBody()->getLinearVelocity();
			btVector3 ReqLVelChange(ReqLVel.x() - CurrLVel.x(), 0.f, ReqLVel.z() - CurrLVel.z());
			if (ReqLVelChange.x() != 0.f || ReqLVelChange.z() != 0.f)
			{
				btVector3 ReqAccel = ReqLVelChange * InvTickTime;
				btScalar AccelMagSq = ReqAccel.length2();
				if (AccelMagSq > MaxAcceleration * MaxAcceleration)
					ReqAccel *= (MaxAcceleration / n_sqrt(AccelMagSq));
				Body->GetBtBody()->applyCentralForce(ReqAccel * Body->GetMass());
			}
			else Body->GetBtBody()->clearForces(); //???really clear all forces?
		}
		else Body->GetBtBody()->setLinearVelocity(ReqLVel);

		// Compensate gravity by a normal force N, as we are standing on the ground
		Body->GetBtBody()->applyCentralForce(-Body->GetBtBody()->getGravity() * Body->GetMass());
		//???!!!compensate ALL -y force? no ground penetration may happen
		//excess force/impulse (force * tick) can be converted into damage or smth!

		// We want to compensate our DistanceToGround in a single simulation step
		const float ReqVerticalVel = -DistanceToGround * InvTickTime;
		Body->GetBtBody()->applyCentralImpulse(btVector3(0.f, ReqVerticalVel * Body->GetMass(), 0.f));
	}
	else
	{
		//???need? gravity should keep the body active
		//if levitating, disable grvity
		//on levitation end, make body active
		//if (!Body->IsAlwaysActive()) Body->MakeAlwaysActive();
	}
}
//---------------------------------------------------------------------

bool CCharacterController::GetLinearVelocity(vector3& Out) const
{
	if (!Body->IsInitialized()) FAIL;
	Out = BtVectorToVector(Body->GetBtBody()->getLinearVelocity());
	OK;
}
//---------------------------------------------------------------------

float CCharacterController::GetAngularVelocity() const
{
	return Body->GetBtBody()->getAngularVelocity().y();
}
//---------------------------------------------------------------------

}
