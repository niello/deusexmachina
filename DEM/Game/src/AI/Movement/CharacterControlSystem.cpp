#include <Game/GameLevel.h>
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <Game/ECS/Components/CharacterControllerComponent.h>
#include <AI/Movement/SteerAction.h>
#include <Physics/PhysicsLevel.h>
#include <Physics/RigidBody.h>
#include <Physics/CollisionShape.h>
#include <Physics/BulletConv.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>

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

//???return IsSelfControlled instead of IsOnGround? Levitate is controlled but above the ground.
static bool UpdateSelfControlState(CCharacterControllerComponent& Character, float DistanceToGround)
{
	auto pBody = Character.Body.Get();
	auto pBtBody = pBody->GetBtBody();
	bool IsOnGround = Character.IsOnTheGround();
	const bool WasOnGround = IsOnGround;

	if (DistanceToGround <= 0.f && !IsOnGround)
	{
		if (Character.State == ECharacterState::Fall)
		{
			//???how to prevent character from taking control over itself until it is recovered from falling?
			//???add ECharacterState::Lay uncontrolled state after a fall or when on the ground? and then recover
			//can even change collision shape for this state (ragdoll?)
		}

		Character.State = ECharacterState::Stand;
		IsOnGround = true;
	}
	else if (DistanceToGround > Character.MaxStepDownHeight && IsOnGround)
	{
		IsOnGround = false;

		//???control whole speed, not only a vertical component?
		// Y is inverted to be positive when the character moves downwards
		const float FallSpeed = -pBtBody->getLinearVelocity().y();
		if (FallSpeed > Character.MaxLandingVerticalSpeed)
		{
			// Can't control itself, fall
			Character.State = ECharacterState::Fall;
			pBody->SetActive(true);
		}
		else if (FallSpeed > 0.f)
		{
			// Landing phase of the jump
			Character.State = ECharacterState::Jump;
			pBody->SetActive(true);
		}
		//???else if VerticalImpulse == 0.f levitate?
	}

	if (IsOnGround == WasOnGround) return IsOnGround;

	// Character on the ground is bound to the surface and uses no gravity
	if (IsOnGround)
	{
		pBtBody->clearGravity();
		pBtBody->setGravity(btVector3(0.f, 0.f, 0.f));
	}
	else
	{
		pBtBody->setGravity(pBody->GetLevel()->GetBtWorld()->getGravity());
		pBtBody->applyGravity();
	}

	return IsOnGround;
}
//---------------------------------------------------------------------

static vector3 ProcessMovement(CCharacterControllerComponent& Character, CActionQueueComponent& Queue, const vector3& Pos)
{
	// Move only when explicitly requested
	auto SteerAction = Queue.FindCurrent<AI::Steer>();
	auto pSteerAction = SteerAction.As<AI::Steer>();
	if (!pSteerAction || Queue.GetStatus(SteerAction) != EActionStatus::Active)
	{
		if (Character.State == ECharacterState::Walk || Character.State == ECharacterState::ShortStep)
			Character.State = ECharacterState::Stand;
		return vector3::Zero;
	}

	vector3 DesiredMovement(pSteerAction->_Dest.x - Pos.x, 0.f, pSteerAction->_Dest.z - Pos.z);

	// Check if already at the desired position
	const float SqDistanceToDest = DesiredMovement.SqLength2D();
	const bool IsSameHeightLevel = (std::fabsf(pSteerAction->_Dest.y - Pos.y) < Character.Height);
	if (IsSameHeightLevel && SqDistanceToDest < AI::Steer::SqLinearTolerance)
	{
		Queue.SetStatus(SteerAction, EActionStatus::Succeeded);
		Character.State = ECharacterState::Stand;
		return vector3::Zero;
	}

	// Select movement mode. Note that ShortStep may turn into Walk if destination
	// becomes farther, but opposite switch Walk -> ShortStep is never performed.
	if (Character.State == ECharacterState::Stand || Character.State == ECharacterState::ShortStep)
	{
		const float DestFacingThreshold = 1.5f * Character.Radius;
		if (IsSameHeightLevel && (SqDistanceToDest <= DestFacingThreshold * DestFacingThreshold))
			Character.State = ECharacterState::ShortStep;
		else
			Character.State = ECharacterState::Walk;
	}

	// If current destination is an intermediate turning point, make the trajectory smooth
	float RemainingDistance = n_sqrt(SqDistanceToDest);
	if (Character.SteeringSmoothness > 0.f && pSteerAction->_NextDest != pSteerAction->_Dest)
	{
		const vector3 ToNext(pSteerAction->_NextDest.x - Pos.x, 0.f, pSteerAction->_NextDest.z - Pos.z);
		const float DistanceToNext = ToNext.Length2D();
		if (DistanceToNext > 0.001f)
		{
			const float Scale = RemainingDistance * Character.SteeringSmoothness / DistanceToNext;
			DesiredMovement -= ToNext * Scale;
			RemainingDistance = DesiredMovement.Length2D();
		}
	}

	// Move with maximal speed when possible
	float Speed = Character.MaxLinearSpeed;

	// Calculate arrival slowdown if close enough to destination.
	// Negative additional distance means that no arrive steering is required at all.
	if (!std::signbit(pSteerAction->_AdditionalDistance))
	{
		// _AdditionalDistance > 0 enables correct arrival when the current destination is not the final one.
		// Navigation system sets _AdditionalDistance to the distance from the requested position to the final
		// destination, and here we calculate effective distance to the destination along the path without
		// path topology information.
		const float Distance = RemainingDistance + pSteerAction->_AdditionalDistance;

		// S = -v0^2/2a for 0 = v0 + at (stop condition)
		const float SlowDownRadius = Character.ArriveBrakingCoeff * Speed * Speed;
		if (Distance < SlowDownRadius)
			Speed *= ((2.f * SlowDownRadius - Distance) * Distance) / (SlowDownRadius * SlowDownRadius);
	}

	// Avoid overshooting, make exactly remaining movement in one frame
	const float FrameTime = Character.Body->GetLevel()->GetStepTime();
	vector3 DesiredLinearVelocity = (RemainingDistance < Speed * FrameTime) ?
		DesiredMovement / FrameTime :
		DesiredMovement * (Speed / RemainingDistance);

// TODO: neighbour separation, obstacle avoidance (see DetourCrowd for impl.)
//if (Character._ObstacleAvoidanceEnabled) AvoidObstacles();

	return DesiredLinearVelocity;
}
//---------------------------------------------------------------------

// Process facing, either explicitly requested or induced by linear movement. Can modify desired linear velocity.
static float ProcessFacing(CCharacterControllerComponent& Character, CActionQueueComponent& Queue, vector3& DesiredLinearVelocity)
{
	// If character can't turn, skip facing
	if (Character.MaxAngularSpeed <= 0.f) return 0.f;

	auto pBody = Character.Body.Get();
	auto pBtBody = pBody->GetBtBody();

	const vector3 LookatDir = BtVectorToVector(pBtBody->getWorldTransform().getBasis() * btVector3(0.f, 0.f, -1.f));

	float DesiredRotation = 0.f;
	if (Character.State == ECharacterState::Walk)
	{
		DesiredRotation = vector3::Angle2DNorm(LookatDir, DesiredLinearVelocity);
	}
	else if (auto TurnAction = Queue.FindCurrent<AI::Turn>())
	{
		auto pTurnAction = TurnAction.As<AI::Turn>();
		if (pTurnAction && Queue.GetStatus(TurnAction) == EActionStatus::Active)
			DesiredRotation = vector3::Angle2DNorm(LookatDir, pTurnAction->_LookatDirection);
	}

	const bool IsNegative = (DesiredRotation < 0.f);
	const float AngleAbs = IsNegative ? -DesiredRotation : DesiredRotation;

	// Already looking at the desired direction
	if (AngleAbs < AI::Turn::AngularTolerance) return 0.f;

	float Speed = Character.MaxAngularSpeed;

	// Calculate arrival slowdown
	constexpr float AngularArrivalZone = 0.34906585f; // 20 degrees in radians
	if (AngleAbs <= AngularArrivalZone)
		Speed *= AngleAbs / AngularArrivalZone;

	// Amount of required rotation is too big, actor must stop moving to perform it
	if (AngleAbs > Character.BigTurnThreshold) DesiredLinearVelocity = vector3::Zero;

	// Avoid overshooting, make exactly remaining rotation in one frame
	const float FrameTime = pBody->GetLevel()->GetStepTime();
	if (AngleAbs < Speed * FrameTime)
		return DesiredRotation / FrameTime;
	else
		return IsNegative ? -Speed : Speed;
}
//---------------------------------------------------------------------

static void UpdateRigidBodyMovement(Physics::CRigidBody* pBody, float dt, const vector3& DesiredLinearVelocity,
	float DesiredAngularVelocity, float MaxAcceleration)
{
	// We want a precise control over the movement, so deny freezing on low speed
	// when movement is requested. When idle, allow to deactivate eventually.
	if (DesiredLinearVelocity != vector3::Zero || DesiredAngularVelocity != 0.f)
		pBody->SetActive(true, true);
	else if (pBody->IsAlwaysActive())
		pBody->SetActive(true, false);

	// No angular acceleration limit, set directly
	pBody->GetBtBody()->setAngularVelocity(btVector3(0.f, DesiredAngularVelocity, 0.f));

	//???what to do with DesiredLinearVelocity.y? perform auto climbing/jumping or deny and wait for an explicit command?
	const btVector3 ReqLinearVelocity = btVector3(DesiredLinearVelocity.x, 0.f, DesiredLinearVelocity.z);
	if (MaxAcceleration <= 0.f)
	{
		// No linear acceleration limit, set directly
		pBody->GetBtBody()->setLinearVelocity(ReqLinearVelocity);
		return;
	}

	btVector3 AccelerationVector = (ReqLinearVelocity - pBody->GetBtBody()->getLinearVelocity()) / dt;
	AccelerationVector.setY(0.f);
	const btScalar SqAcceleration = AccelerationVector.length2();
	if (SqAcceleration > MaxAcceleration * MaxAcceleration)
		AccelerationVector *= (MaxAcceleration / n_sqrt(SqAcceleration));
	pBody->GetBtBody()->applyCentralForce(AccelerationVector * pBody->GetMass());
}
//---------------------------------------------------------------------

void ProcessCharacterControllers(CGameWorld& World, Physics::CPhysicsLevel& PhysicsLevel, float dt)
{
	World.ForEachEntityWith<CCharacterControllerComponent, CActionQueueComponent>(
		[dt, &PhysicsLevel](auto EntityID, auto& Entity,
			CCharacterControllerComponent& Character,
			CActionQueueComponent& Queue)
	{
		auto pBody = Character.Body.Get();
		if (!pBody || pBody->GetLevel() != &PhysicsLevel) return;

		// Access real physical transform, not an interpolated motion state
		const auto& Offset = pBody->GetCollisionShape()->GetOffset();
		const vector3 Pos = BtVectorToVector(pBody->GetBtBody()->getWorldTransform() * btVector3(-Offset.x, -Offset.y, -Offset.z));

		const float DistanceToGround = CalcDistanceToGround(Character, Pos);
		if (UpdateSelfControlState(Character, DistanceToGround))
		{
			// Update movement and other self-control
			vector3 DesiredLinearVelocity = ProcessMovement(Character, Queue, Pos);
			const float DesiredAngularVelocity = ProcessFacing(Character, Queue, DesiredLinearVelocity);
			UpdateRigidBodyMovement(pBody, dt, DesiredLinearVelocity, DesiredAngularVelocity, Character.MaxAcceleration);

			// TODO: not needed when levitate, only when really stand on the ground
			// We stand on the ground and want to compensate our DistanceToGround in a single simulation step
			pBody->GetBtBody()->applyCentralImpulse(btVector3(0.f, (-DistanceToGround / dt) * pBody->GetMass(), 0.f));
		}
		else
		{
			// TODO: update above the ground (uncontrolled) state
		}
	});
}
//---------------------------------------------------------------------

void CheckCharacterControllersArrival(CGameWorld& World, Physics::CPhysicsLevel& PhysicsLevel)
{
	World.ForEachEntityWith<CCharacterControllerComponent, CActionQueueComponent>(
		[&PhysicsLevel](auto EntityID, auto& Entity,
			CCharacterControllerComponent& Character,
			CActionQueueComponent& Queue)
	{
		auto pBody = Character.Body.Get();
		if (!pBody || pBody->GetLevel() != &PhysicsLevel) return;

		// Access real physical transform, not an interpolated motion state
		const auto& BodyTfm = pBody->GetBtBody()->getWorldTransform();

		// NB: we don't try to process Steer and Turn simultaneously, only the most nested of them
		auto Action = Queue.FindCurrent<AI::Steer, AI::Turn>();

		if (auto pSteerAction = Action.As<AI::Steer>())
		{
			// Check linear arrival
			const auto& Offset = pBody->GetCollisionShape()->GetOffset();
			const vector3 Pos = BtVectorToVector(BodyTfm * btVector3(-Offset.x, -Offset.y, -Offset.z));
			const float SqDistance = vector3::SqDistance2D(pSteerAction->_Dest, Pos);
			const bool IsSameHeightLevel = (std::fabsf(pSteerAction->_Dest.y - Pos.y) < Character.Height);
			if (IsSameHeightLevel && SqDistance < AI::Steer::SqLinearTolerance)
			{
				Queue.SetStatus(Action, EActionStatus::Succeeded);
				if (Character.State == ECharacterState::Walk || Character.State == ECharacterState::ShortStep)
					Character.State = ECharacterState::Stand;
			}
			else
			{
				// TODO:
				// If is stuck, increase stuck timer, and if timer is too high, set Stuck mvmt state, i.e. fail movement.
				// Stuck can be detected if resulting moving average distance remains almost the same for some period of time.
			}
		}
		else if (auto pTurnAction = Action.As<AI::Turn>())
		{
			// Check angular arrival
			const vector3 LookatDir = BtVectorToVector(BodyTfm.getBasis() * btVector3(0.f, 0.f, -1.f));
			const float Angle = vector3::Angle2DNorm(LookatDir, pTurnAction->_LookatDirection);
			if (std::fabsf(Angle) < AI::Turn::AngularTolerance)
				Queue.SetStatus(Action, EActionStatus::Succeeded);
		}
	});
}
//---------------------------------------------------------------------

void UpdateCharacterControllerShape(CCharacterControllerComponent& Character)
{
	// FIXME: use dirty flag or check changes in shape & mass?
	// NB: can reuse body, setCollisionShape + setMass!

	auto pBody = Character.Body.Get();

	Physics::CPhysicsLevel* pLevel = nullptr;
	Scene::CSceneNode* pNode = nullptr;
	matrix44 Tfm;
	if (pBody)
	{
		pLevel = pBody->GetLevel();
		pNode = pBody->GetControlledNode();
		pBody->GetTransform(Tfm);
		pBody->RemoveFromLevel();
	}

	// FIXME PHYSICS - where to set? In component (externally)?
	CStrID CollisionGroupID("Character");
	CStrID CollisionMaskID("All");

	const float CapsuleHeight = Character.Height - Character.Radius - Character.Radius - Character.Hover;
	n_assert(CapsuleHeight > 0.f);
	const vector3 Offset(0.f, (Character.Hover + Character.Height) * 0.5f, 0.f);
	auto Shape = Physics::CCollisionShape::CreateCapsuleY(Character.Radius, CapsuleHeight, Offset);

	Character.Body = n_new(Physics::CRigidBody(Character.Mass, *Shape, CollisionGroupID, CollisionMaskID, Tfm));
	pBody = Character.Body.Get();

	pBody->GetBtBody()->setAngularFactor(btVector3(0.f, 1.f, 0.f));

	if (pNode) pBody->SetControlledNode(pNode);
	if (pLevel) pBody->AttachToLevel(*pLevel);
}
//---------------------------------------------------------------------

/*
//!!!FIXME: what if obstacle is over the destination point? brake and stand stuck until request is failed or location if freed?
void CCharacterController::AvoidObstacles()
{
	constexpr float ObstacleDetectionMinRadius = 0.1f;
	constexpr float ObstaclePredictionTime = 1.2f;
	const float DetectionRadius = ObstacleDetectionMinRadius + Speed * ObstaclePredictionTime;

#ifdef DETOUR_OBSTACLE_AVOIDANCE // In ActorFwd.h
	float Range = pActor->Radius + DetectionRadius;

	//???here or global persistent?
	dtObstacleAvoidanceQuery ObstacleQuery;
	ObstacleQuery.init(6, 8);
	//ObstacleQuery.reset();
	pActor->GetNavSystem().GetObstacles(Range, ObstacleQuery);

	//???pre-filter mem facts? distance < Range, height intersects actor height etc
	CMemFactNode It = pActor->GetMemSystem().GetFactsByType(CMemFactObstacle::RTTI);
	for (; It; ++It)
	{
		CMemFactObstacle* pObstacle = (CMemFactObstacle*)It->Get();
		//!!!remember obstacle velocity in the fact and use here!
		// desired velocity can be get from obstacles-actors only!
		ObstacleQuery.addCircle(pObstacle->Position.v, pObstacle->Radius, vector3::Zero.v, vector3::Zero.v);
	}

	//!!!GET AT ACTIVATION AND STORE!
	const dtObstacleAvoidanceParams* pOAParams = AISrv->GetDefaultObstacleAvoidanceParams();

	if (ObstacleQuery.getObstacleCircleCount() || ObstacleQuery.getObstacleSegmentCount())
	{
		vector3 LinearVel;
		pActor->GetEntity()->GetAttr(LinVel, CStrID("LinearVelocity"));
		float DesVel[3] = { LinearVel.x, 0.f, LinearVel.z }; // Copy LVel because it is modified inside the call
		if (AdaptiveVelocitySampling)
			ObstacleQuery.sampleVelocityAdaptive(pActor->Position.v, pActor->Radius, MaxSpeed[pActor->MvmtType],
													Velocity.v, DesVel, LinearVel.v, pOAParams);
		else
			ObstacleQuery.sampleVelocityGrid(pActor->Position.v, pActor->Radius, MaxSpeed[pActor->MvmtType],
												Velocity.v, DesVel, LinearVel.v, pOAParams);
	}

#else
	CMemFactNode It = pActor->GetMemSystem().GetFactsByType(CMemFactObstacle::RTTI);

	//!!!???dynamic obstacles - take velocity into account!?
	//!!!read about VO & RVO!
	//!!!if get stuck avoiding obstacle, try to select far point

	if (It)
	{
		CMemFactObstacle* pClosest = nullptr;
		float MinIsect = FLT_MAX;
		float MinExpRadius;

		vector2 ActorPos(pActor->Position.x, pActor->Position.z);
		vector2 ToDest(DestPoint.x - ActorPos.x, DestPoint.z - ActorPos.y);
		float DistToDest = ToDest.Length();
		ToDest /= DistToDest;
		vector2 ActorSide(-ToDest.y, ToDest.x);
		float ActorRadius = pActor->Radius + AvoidanceMargin;

		float DetectorLength = ActorRadius + DetectionRadius;

		for (; It; ++It)
		{
			CMemFactObstacle* pObstacle = (CMemFactObstacle*)It->Get();

			// Uncomment if obstacle has Height
			//if ((pActor->Position.y + pActor->Height < pObstacle->Position.y) ||
			//	(pObstacle->Position.y + pObstacle->Height < pActor->Position.y))
			//	continue;

			vector2 FromObstacleToDest(
				DestPoint.x - pObstacle->Position.x,
				DestPoint.z - pObstacle->Position.z);
			float ExpRadius = ActorRadius + pObstacle->Radius;
			float SqExpRadius = ExpRadius * ExpRadius;

			// Don't avoid obstacles at the destination
			if (FromObstacleToDest.SqLength() < SqExpRadius) continue;

			vector2 ToObstacle(
				pObstacle->Position.x - ActorPos.x,
				pObstacle->Position.z - ActorPos.y);
			float SqDistToObstacle = ToObstacle.SqLength();

			if (SqDistToObstacle >= DistToDest * DistToDest) continue; 

			float DetectRadius = DetectorLength + pObstacle->Radius;

			if (SqDistToObstacle >= DetectRadius * DetectRadius) continue;

			vector2 LocalPos(pObstacle->Position.x, pObstacle->Position.z);
			LocalPos.ToLocalAsPoint(ToDest, ActorSide, ActorPos);

			if (LocalPos.x <= 0.f || n_fabs(LocalPos.y) >= ExpRadius) continue;

			float SqrtPart = sqrtf(SqExpRadius - LocalPos.y * LocalPos.y);
			float Isect = (LocalPos.x <= SqrtPart) ? LocalPos.x + SqrtPart : LocalPos.x - SqrtPart;

			if (Isect < MinIsect)
			{
				pClosest = pObstacle;
				MinIsect = Isect;
				MinExpRadius = ExpRadius;
			}
		}

		if (pClosest)
		{
			// Will not work if raduis of obstacle can change. If so, check Radius too.
			vector2 ObstaclePos(pClosest->Position.x, pClosest->Position.z);
			if (pClosest == pLastClosestObstacle && LastClosestObstaclePos.isequal(ObstaclePos, 0.01f))
				ToDest = LastAvoidDir;
			else
			{
				vector2 ToObstacle = ObstaclePos - ActorPos;
				vector2 ProjDir = ToDest * ToObstacle.dot(ToDest) - ToObstacle;
				float ProjDirLen = ProjDir.Length();
				if (ProjDirLen <= TINY)
				{
					ProjDir.set(-ToObstacle.y, ToObstacle.x);
					ProjDirLen = ProjDir.Length();
				}

				ToDest = ToObstacle + ProjDir * (MinExpRadius / ProjDirLen);
				ToDest.norm();
				LastAvoidDir = ToDest;
			}

			//!!!SPEED needs adjusting to! brake or arrival effect elimination.
			LinearVel.set(ToDest.x * Speed, 0.f, ToDest.y * Speed);

			LastClosestObstaclePos = ObstaclePos;
		}

		pLastClosestObstacle = pClosest;
	}
#endif
}
//---------------------------------------------------------------------

bool CMotorSystem::IsStuck()
{
	// Set Stuck, if:
	// too much time with too little movement, but far enough from the dest (RELATIVELY little movement)
	// or
	// too much time with no RELATIVELY significant progress to dest (distance doesn't reduce)
	// second is more general
	//!!!if we want to Flee, stuck must check growing of distance to dest, not reduction!
	//???do we really want to flee sometimes? hm, returning to weapon radius?
	//may be just take into account reach radii?

	//!!!DON'T forget StuckTime!

	FAIL;
}
//---------------------------------------------------------------------

void CMotorSystem::RenderDebug(Debug::CDebugDraw& DebugDraw)
{
	static const vector4 ColorNormal(1.0f, 1.0f, 1.0f, 1.0f);
	static const vector4 ColorStuck(1.0f, 0.0f, 0.0f, 1.0f);

	//???draw cross?
	if (pActor->MvmtState == AIMvmt_DestSet || pActor->MvmtState == AIMvmt_Stuck)
		DebugDraw.DrawLine(
			DestPoint,
			vector3(DestPoint.x, DestPoint.y + 1.f, DestPoint.z),
			pActor->MvmtState == AIMvmt_DestSet ? ColorNormal : ColorStuck);
 
	CMemFactNode It = pActor->GetMemSystem().GetFactsByType(CMemFactObstacle::RTTI);
	for (; It; ++It)
	{
		CMemFactObstacle* pObstacle = (CMemFactObstacle*)It->Get();
		matrix44 Tfm;
		Tfm.set_translation(pObstacle->Position);
		vector4 Color(0.6f, 0.f, 0.8f, 0.5f * pObstacle->Confidence);
		if (pObstacle == pLastClosestObstacle)
		{
			Color.x = 0.4f;
			Color.z = 0.9f;
		}
		DebugDraw.DrawCylinder(Tfm, pObstacle->Radius, 1.f, Color); // pObstacle->Height instead of 1.f
	}

	const char* pMvmt = nullptr;
	if (pActor->MvmtState == AIMvmt_Failed) pMvmt = "None";
	else if (pActor->MvmtState == AIMvmt_Done) pMvmt = "Done";
	else if (pActor->MvmtState == AIMvmt_DestSet) pMvmt = "DestSet";
	else if (pActor->MvmtState == AIMvmt_Stuck) pMvmt = "Stuck";

	CString Text;
	Text.Format("Mvmt state: %s\nFace direction set: %s\n", pMvmt, pActor->FacingState == AIFacing_DirSet ? "true" : "false");

	if (pActor->MvmtState == AIMvmt_DestSet)
	{
		vector2 ToDest(DestPoint.x - pActor->Position.x, DestPoint.z - pActor->Position.z);

		CString Text2;
		Text2.Format("DestPoint: %.4f, %.4f, %.4f\n"
			"Position: %.4f, %.4f, %.4f\n"
			"DistToDest: %.4f\n"
			"MinReach: %.4f\n"
			"MaxReach: %.4f\n",
			DestPoint.x,
			DestPoint.y,
			DestPoint.z,
			pActor->Position.x,
			pActor->Position.y,
			pActor->Position.z,
			ToDest.Length());
		Text += Text2;
	}

	//DebugDraw->DrawText(Text.CStr(), 0.05f, 0.1f);
}
//---------------------------------------------------------------------

bool CPropCharacterController::OnRenderDebug(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	if (!IsEnabled()) OK;

	static const vector4 ColorVel(1.0f, 0.5f, 0.0f, 1.0f);
	static const vector4 ColorReqVel(0.0f, 1.0f, 1.0f, 1.0f);

	vector3 Pos;
	quaternion Rot;
	CharCtlr->GetBody()->GetTransform(Pos, Rot);

	matrix44 Tfm;
	Tfm.FromQuaternion(Rot);
	Tfm.Translation() = Pos;

	DebugDraw->DrawCoordAxes(Tfm);
	vector3 LinVel;
	CharCtlr->GetLinearVelocity(LinVel);
	DebugDraw->DrawLine(Tfm.Translation(), Tfm.Translation() + LinVel, ColorVel);
	DebugDraw->DrawLine(Tfm.Translation(), Tfm.Translation() + CharCtlr->GetRequestedLinearVelocity(), ColorReqVel);

	if (EntityID is in selection && CharCtlr->IsMotionRequested()) //!!!write debug focus or smth!
	{
		vector3 LVel = CharCtlr->GetLinearVelocity();

		CString Text;
		Text.Format("\n\n\n\n\n\n\n\n"
			"Requested velocity: %.4f, %.4f, %.4f\n"
			"Actual velocity: %.4f, %.4f, %.4f\n"
			"Requested angular velocity: %.5f\n"
			"Actual angular velocity: %.5f\n"
			"Requested speed: %.4f\n"
			"Actual speed: %.4f\n",
			CharCtlr->GetRequestedLinearVelocity().x,
			CharCtlr->GetRequestedLinearVelocity().y,
			CharCtlr->GetRequestedLinearVelocity().z,
			LVel.x,
			LVel.y,
			LVel.z,
			CharCtlr->GetRequestedAngularVelocity(),
			CharCtlr->GetAngularVelocity(),
			CharCtlr->GetRequestedLinearVelocity().Length(),
			LVel.Length());
		//DebugDraw->DrawText(Text.CStr(), 0.05f, 0.1f);
	}

	OK;
}
//---------------------------------------------------------------------
*/

}
