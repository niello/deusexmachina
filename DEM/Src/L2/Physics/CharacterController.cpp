#include "CharacterController.h"

#include <Physics/BulletConv.h>
#include <Physics/PhysicsServer.h>
#include <Physics/PhysicsWorld.h>
#include <Physics/RigidBody.h>
#include <Physics/ClosestNotMeRayResultCallback.h>
#include <Data/Params.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>

namespace Physics
{
//__ImplementClass(Physics::CCharacterController, 'CCTL', Core::CRefCounted);

bool CCharacterController::Init(const Data::CParams& Desc)
{
	Radius = Desc.Get<float>(CStrID("Radius"), 0.3f);
	Height = Desc.Get<float>(CStrID("Height"), 1.75f);
	Hover = Desc.Get<float>(CStrID("Hover"), 0.2f);
	float Mass = Desc.Get<float>(CStrID("Mass"), 80.f);

	float CapsuleHeight = Height - Radius - Radius - Hover;
	n_assert (CapsuleHeight > 0.f);

	PCollisionShape Shape = PhysicsSrv->CreateCapsuleShape(Radius, CapsuleHeight);

	vector3 Offset(0.f, (Hover + Height) * 0.5f, 0.f);

	Body = n_new(CRigidBody);
	Body->Init(*Shape, Mass, 0x0001, 0xffff, Offset); //!!!need normal flags!
	Body->GetBtBody()->setAngularFactor(btVector3(0.f, 1.f, 0.f));

	//???or activate only when command is received?
	//???set low threshold instead?
	Body->GetBtBody()->setActivationState(DISABLE_DEACTIVATION);

	MaxAcceleration = 0.f;
	ReqLinVel = vector3::Zero;
	ReqAngVel = 0.f;

	OK;
}
//---------------------------------------------------------------------

void CCharacterController::Term()
{
	RemoveFromLevel();
	Body = NULL;
}
//---------------------------------------------------------------------

bool CCharacterController::AttachToLevel(CPhysicsWorld& World)
{
	return Body.IsValid() && Body->AttachToLevel(World);
}
//---------------------------------------------------------------------

void CCharacterController::RemoveFromLevel()
{
	if (Body.IsValid()) Body->RemoveFromLevel();
}
//---------------------------------------------------------------------

bool CCharacterController::IsAttachedToLevel() const
{
	return Body.IsValid() && Body->IsAttachedToLevel();
}
//---------------------------------------------------------------------

void CCharacterController::Update()
{
	if (!IsAttachedToLevel()) return;

	vector3 Pos;
	quaternion Rot;
	Body->GetTransform(Pos, Rot);

	btVector3 BtStart = VectorToBtVector(Pos);
	btVector3 BtEnd = BtStart;
	BtStart.m_floats[1] += Height;
	BtEnd.m_floats[1] -= 0.5f; //!!!- MaxStepDown - BelowProbeLength!
	CClosestNotMeRayResultCallback RayCB(BtStart, BtEnd, Body->GetBtObject());
	Body->GetWorld()->GetBtWorld()->rayTest(BtStart, BtEnd, RayCB);

	float DistanceToGround = RayCB.hasHit() ? Pos.y - RayCB.m_hitPointWorld.y() : FLT_MAX;

	//!!!take step down height into account, if above the ground by less\
	//than StepDown, use ground binding instead of falling!

	//!!!if jumping(?) or falling, don't use step down!

	// Process jumping or falling
	if (DistanceToGround > 0.f) //!!!if > MaxStepDown!
	{
		//???!!!apply damping?!
		return;
	}

	//!!!If body auto deactivation is enabled, activate here if request affects it!

	// No angular acceleration limit, set directly
	Body->GetBtBody()->setAngularVelocity(btVector3(0.f, ReqAngVel, 0.f));

	//btScalar CurrLinVelY = Body->GetBtBody()->getLinearVelocity().y();
	//btVector3 ReqLVel = btVector3(ReqLinVel.x, CurrLinVelY, ReqLinVel.z);
	btVector3 ReqLVel = btVector3(ReqLinVel.x, 0.f, ReqLinVel.z);
	if (MaxAcceleration > 0.f)
	{
		const btVector3& CurrLVel = Body->GetBtBody()->getLinearVelocity();
		if (CurrLVel.x() != ReqLVel.x() || CurrLVel.z() != ReqLVel.z())
		{
			// calc acceleration from velocity
			// limit acceleration vector magnitude only in XZ plane
			// create force as F = a * m
			// apply central force
		}
	}
	else Body->GetBtBody()->setLinearVelocity(ReqLVel);

	// Now our linear velocity has no vertical component, and we aren't jumping or falling.
	// We want to offset to DistanceToGround in a single simulation step.
	//vector3 Gvt;
	//Body->GetWorld()->GetGravity(Gvt);
	//const float ReqVerticalVel = -DistanceToGround / Body->GetWorld()->GetStepTime() - Gvt.y * Body->GetWorld()->GetStepTime();
	const float ReqVerticalVel = -DistanceToGround / Body->GetWorld()->GetStepTime();
	Body->GetBtBody()->applyCentralImpulse(btVector3(0.f, ReqVerticalVel * Body->GetMass(), 0.f));

	/*
	if (BodyIsEnabled && !Actuated && DistanceToGround > -0.002f)
	{
		const float FreezeThreshold = 0.00001f; //???use TINY?

		bool AVelIsAlmostZero = n_fabs(AngularVel.y) < FreezeThreshold;
		bool LVelIsAlmostZero = n_fabs(LinearVel.x) * (float)Level->GetStepSize() < FreezeThreshold &&
								n_fabs(LinearVel.z) * (float)Level->GetStepSize() < FreezeThreshold;

		if (AVelIsAlmostZero)
			pMasterBody->SetAngularVelocity(vector3::Zero);

		if (LVelIsAlmostZero)
			pMasterBody->SetLinearVelocity(vector3::Zero);

		if (AVelIsAlmostZero && LVelIsAlmostZero) SetEnabled(false);
	}
	*/
}
//---------------------------------------------------------------------

bool CCharacterController::GetLinearVelocity(vector3& Out) const
{
	if (!Body->IsInitialized()) FAIL;
	Out = BtVectorToVector(Body->GetBtBody()->getLinearVelocity());
	OK;
}
//---------------------------------------------------------------------

}
