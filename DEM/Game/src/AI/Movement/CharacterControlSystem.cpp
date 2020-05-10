#include <Game/GameLevel.h>
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <Game/ECS/Components/CharacterControllerComponent.h>
#include <AI/Movement/SteeringController.h>
#include <Physics/PhysicsLevel.h>
#include <Physics/RigidBody.h>
#include <Physics/CollisionShape.h>
#include <Physics/BulletConv.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>

namespace DEM::Game
{

static float CalcDistanceToGround(const CCharacterControllerComponent& Character, const vector3& Pos)
{
	constexpr float GroundProbeLength = 0.5f;

	vector3 Start = Pos;
	vector3 End = Pos;
	Start.y += Character.Height;
	End.y -= (Character.MaxStepDownHeight + GroundProbeLength); // Falling state detection

	auto pBody = Character.Body.Get();

	// FIXME: improve passing collision flags through interfaces!
	const auto* pCollisionProxy = pBody->GetBtBody()->getBroadphaseProxy();

	vector3 ContactPos;
	if (pBody->GetLevel()->GetClosestRayContact(Start, End,
			pCollisionProxy->m_collisionFilterGroup,
			pCollisionProxy->m_collisionFilterMask,
			&ContactPos, nullptr, pBody))
	{
		return Pos.y - ContactPos.y;
	}

	return std::numeric_limits<float>().max();
}
//---------------------------------------------------------------------

static vector3 CalcDesiredLinearVelocity(const CCharacterControllerComponent& Character, const vector3& Pos, const DEM::AI::Steer& Request)
{
	// Calculate desired movement vector

	vector3 DesiredMovement(Request._Dest.x - Pos.x, 0.f, Request._Dest.z - Pos.z);

	// If current destination is an intermediate turning point, make the trajectory smooth
	if (Character.SteeringSmoothness > 0.f && Request._NextDest != Request._Dest)
	{
		const vector3 ToNext(Request._NextDest.x - Pos.x, 0.f, Request._NextDest.z - Pos.z);
		const float DistanceToNext = ToNext.Length2D();
		if (DistanceToNext > 0.001f)
		{
			const float Scale = DesiredMovement.Length2D() * Character.SteeringSmoothness / DistanceToNext;
			DesiredMovement -= ToNext * Scale;
		}
	}

	const float RemainingDistance = DesiredMovement.Length2D();
	if (RemainingDistance <= std::numeric_limits<float>().epsilon())
		return vector3::Zero;

	// Calculate speed

	float Speed = Character.MaxLinearSpeed;

	// Calculate arrival slowdown if close enough to destination
	// Negative _ArriveAdditionalDistance means that no arrive steering required at all
	if (Request._AdditionalDistance >= 0.f)
	{
		// _AdditionalArriveDistance > 0 enables correct arrival when the current destination is not the final one.
		// Navigation system sets _AdditionalArriveDistance to the distance from the requested position to the
		// final destination, and here we calculate effective distance to the destination along the path without
		// path topology information.
		const float Distance = RemainingDistance + Request._AdditionalDistance;

		// S = -v0^2/2a for 0 = v0 + at (stop condition)
		const float SlowDownRadius = Character.ArriveBrakingCoeff * Speed * Speed;
		if (Distance < SlowDownRadius)
			Speed *= ((2.f * SlowDownRadius - Distance) * Distance) / (SlowDownRadius * SlowDownRadius);
	}

	// Avoid overshooting, make exactly remaining movement in one frame
	const float FrameTime = Character.Body->GetLevel()->GetStepTime();
	if (RemainingDistance < Speed * FrameTime) return DesiredMovement / FrameTime;

	// Calculate velocity as speed * normalized movement direction
	return DesiredMovement * (Speed / RemainingDistance);
}
//---------------------------------------------------------------------

static float CalcDesiredAngularVelocity(const CCharacterControllerComponent& Character, float Angle)
{
	constexpr float AngularArrivalZone = 0.34906585039886591538473815369772f; // 20 degrees in radians

	const bool IsNegative = (Angle < 0.f);
	const float AngleAbs = IsNegative ? -Angle : Angle;

	float Speed = Character.MaxAngularSpeed;

	// Calculate arrival slowdown
	if (AngleAbs <= AngularArrivalZone)
		Speed *= AngleAbs / AngularArrivalZone;

	// Avoid overshooting, make exactly remaining rotation in one frame
	const float FrameTime = Character.Body->GetLevel()->GetStepTime();
	if (AngleAbs < Speed * FrameTime) return Angle / FrameTime;

	return IsNegative ? -Speed : Speed;
}
//---------------------------------------------------------------------

void ProcessCharacterControllers(DEM::Game::CGameWorld& World, Physics::CPhysicsLevel& PhysicsLevel, float dt)
{
	World.ForEachEntityWith<CCharacterControllerComponent, DEM::Game::CActionQueueComponent>(
		[dt, &PhysicsLevel](auto EntityID, auto& Entity,
			CCharacterControllerComponent& Character,
			DEM::Game::CActionQueueComponent* pQueue)
	{
		auto pBody = Character.Body.Get();
		if (!pBody || pBody->GetLevel() != &PhysicsLevel) return;

		auto pBtBody = pBody->GetBtBody();

		// synchronizeMotionStates() might be not yet called, so we access raw transform.
		// synchronizeSingleMotionState() could make sense too if it has dirty flag optimization,
		// then called here it would not do redundant calculations in synchronizeMotionStates.
		const auto OffsetCompensation = -pBody->GetCollisionShape()->GetOffset();
		const auto& BodyTfm = pBtBody->getWorldTransform();
		const vector3 Pos = BtVectorToVector(BodyTfm * btVector3(OffsetCompensation.x, OffsetCompensation.y, OffsetCompensation.z));
		const vector3 LookatDir = BtVectorToVector(BodyTfm.getBasis() * btVector3(0.f, 0.f, -1.f));

		//!!!FIXME: check motion state, maybe it is up to date in BeforePhysicsTick!

		// Update vertical state

		const float DistanceToGround = CalcDistanceToGround(Character, Pos);
		const bool OnGround = Character.IsOnTheGround();

		if (DistanceToGround <= 0.f && !OnGround)
		{
			if (Character.State == ECharacterState::Fall)
			{
				//???how to prevent character from taking control over itself until it is recovered from falling?
				//???add ECharacterState::Lay uncontrolled state after a fall or when on the ground? and then recover
				//can even change collision shape for this state (ragdoll?)
			}

			Character.State = ECharacterState::Stand;
		}
		else if (DistanceToGround > Character.MaxStepDownHeight && OnGround)
		{
			//???control whole speed or only a vertical component? now the second

			// Y is inverted to be positive when the character moves downwards
			const float VerticalImpulse = pBody->GetMass() * -pBtBody->getLinearVelocity().y();
			if (VerticalImpulse > Character.MaxLandingImpulse)
			{
				Character.State = ECharacterState::Fall;
				pBody->SetActive(true);
			}
			else if (VerticalImpulse > 0.f)
			{
				Character.State = ECharacterState::Jump;
				pBody->SetActive(true);
			}
			//???else if VerticalImpulse == 0.f levitate?
		}

		// Update the controller in the current state, including movement requests

		if (Character.State == ECharacterState::Stand)
		{
			vector3 DesiredLinearVelocity;

			if (auto pSteerAction = pQueue->FindActive<DEM::AI::Steer>())
			{
				//???set EMovementState::Requested and some initial params if was idle?
				// _NeedDestinationFacing must be determined once, not every frame!
				//???use different states for it instead of bool? Moving / ShortStep

				//!!!if lookat requested here (RequestFacing below), must finish request
				//even if stopped moving before! If cancelled movement, cancel facing too.

				DesiredLinearVelocity = CalcDesiredLinearVelocity(Character, Pos, *pSteerAction);

				// See DetourCrowd for impl.
				//!!!Can add separation with neighbours here too!
				//if (Character._ObstacleAvoidanceEnabled) AvoidObstacles();

				if (NeedDestinationFacing && Character.MaxAngularSpeed > 0.f)
					RequestFacing(DesiredLinearVelocity);
			}

			float DesiredAngularVelocity = 0.f;
			if (_AngularMovementState == EMovementState::Requested)
			{
				const float Angle = vector3::Angle2DNorm(LookatDir, _RequestedLookat);
				DesiredAngularVelocity = CalcDesiredAngularVelocity(Angle);

				// Amount of required rotation is too big, actor must stop moving to perform it
				if (std::fabsf(Angle) > Character.BigTurnThreshold) DesiredLinearVelocity = vector3::Zero;
			}

			// We want a precise control over the movement, so deny freezing on low speed
			// when movement is requested. When idle, allow to deactivate eventually.
			if (DesiredLinearVelocity != vector3::Zero || DesiredAngularVelocity != 0.f)
				pBody->SetActive(true, true);
			else if (pBody->IsAlwaysActive())
				pBody->SetActive(true, false);

			// No angular acceleration limit, set directly
			pBtBody->setAngularVelocity(btVector3(0.f, DesiredAngularVelocity, 0.f));

			const float InvTickTime = 1.f / dt;

			//???what to do with requested y? perform auto climbing/jumping or deny and wait for an explicit command?
			const btVector3 ReqLVel = btVector3(DesiredLinearVelocity.x, 0.f, DesiredLinearVelocity.z);
			if (Character.MaxAcceleration > 0.f)
			{
				const btVector3& CurrLVel = pBtBody->getLinearVelocity();
				btVector3 ReqLVelChange(ReqLVel.x() - CurrLVel.x(), 0.f, ReqLVel.z() - CurrLVel.z());
				if (ReqLVelChange.x() != 0.f || ReqLVelChange.z() != 0.f)
				{
					btVector3 ReqAccel = ReqLVelChange * InvTickTime;
					btScalar AccelMagSq = ReqAccel.length2();
					if (AccelMagSq > Character.MaxAcceleration * Character.MaxAcceleration)
						ReqAccel *= (Character.MaxAcceleration / n_sqrt(AccelMagSq));
					pBtBody->applyCentralForce(ReqAccel * pBody->GetMass());
				}
				else pBtBody->clearForces(); //???really clear all forces?
			}
			else pBtBody->setLinearVelocity(ReqLVel);

			// Compensate gravity by a normal force N, as we are standing on the ground
			pBtBody->applyCentralForce(-pBtBody->getGravity() * pBody->GetMass());
			//???!!!compensate ALL -y force? no ground penetration may happen
			//excess force/impulse (force * tick) can be converted into damage or smth!

			// We want to compensate our DistanceToGround in a single simulation step
			const float ReqVerticalVel = -DistanceToGround * InvTickTime;
			pBtBody->applyCentralImpulse(btVector3(0.f, ReqVerticalVel * pBody->GetMass(), 0.f));
		}
		else
		{
			//???need? gravity should keep the body active
			//if levitating, disable gravity
			//on levitation end, make body active
			//pBody->SetActive(true, true);
		}
	});
}
//---------------------------------------------------------------------

}
