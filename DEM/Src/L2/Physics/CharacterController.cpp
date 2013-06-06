#include "CharacterController.h"

#include <Game/GameServer.h>
#include <Physics/PhysicsServer.h>
#include <Physics/PhysicsWorld.h>
#include <Physics/RigidBody.h>
#include <Data/Params.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>

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
	Pos.y += Height;

	//!!!REQUEST closest not me below!
	/*
	Game::CSurfaceInfo SurfInfo;
	float DistanceToGround;
	if (Body->GetWorld()->GetSurfaceInfoUnder(SurfInfo, Pos, Height + 0.1f)) //!!!Height + MaxStepDown!
	{
		DistanceToGround = Pos.y - Height - SurfInfo.WorldHeight;
	}
	else
	{
		DistanceToGround = FLT_MAX;
	}
	*/
	float DistanceToGround = 0.f;

	//!!!take step down height into account, if above the ground by less\
	//than StepDown, use ground binding instead of falling!

	// Process jumping or falling
	if (DistanceToGround > 0.f) //!!!if > MaxStepDown!
	{
		//???!!!apply damping?!
		return;
	}

	//!!!If body auto deactivation is enabled, activate here if request affects it!

	// No angular acceleration limit, set directly
	Body->GetBtBody()->setAngularVelocity(btVector3(0.f, ReqAngVel, 0.f));

	btVector3 ReqLVel = btVector3(ReqLinVel.x, 0.f, ReqLinVel.z);
	if (MaxAcceleration > 0.f)
	{
		const btVector3& CurrLVel = Body->GetBtBody()->getLinearVelocity();
		if (CurrLVel.x() != ReqLVel.x() || CurrLVel.z() != ReqLVel.z())
		{
			// calc acceleration from velocity
			// limit acceleration vector magnitude
			// create force as F = a * m
			// apply central force
		}
	}
	else Body->GetBtBody()->setLinearVelocity(ReqLVel);

	// Now our linear velocity has no vertical component, and we aren't jumping or falling.
	// We want to offset to DistanceToGround in a single simulation step.
	const float ReqVerticalVel = DistanceToGround / Body->GetWorld()->GetStepTime();
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

}
