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

	// synchronizeMotionStates() might be not yet called, so we access raw transform.
	// synchronizeSingleMotionState() could make sense too if it has dirty flag optimization,
	// then called here it would not do redundant calculations in synchronizeMotionStates.
	const auto OffsetCompensation = -_Body->GetCollisionShape()->GetOffset();
	const auto& BodyTfm = _Body->GetBtBody()->getWorldTransform();
	const vector3 Pos = BtVectorToVector(BodyTfm * btVector3(OffsetCompensation.x, OffsetCompensation.y, OffsetCompensation.z));
	const vector3 LookatDir = BtVectorToVector(BodyTfm.getBasis() * btVector3(0.f, 0.f, -1.f));

	// Update vertical state

	const float DistanceToGround = CalcDistanceToGround(Pos);
	const bool OnGround = IsOnTheGround();

	if (DistanceToGround <= 0.f && !OnGround)
	{
		if (_State == ECharacterState::Jump)
		{
			// send event OnEndJumping (//???through callback?)
		}
		else if (_State == ECharacterState::Fall)
		{
			// send event OnEndFalling (//???through callback?)
			//???how to prevent character from taking control over itself until it is recovered from falling?
			//???add ECharacterState::Lay uncontrolled state after a fall or when on the ground? and then recover
			//can even change collision shape for this state (ragdoll?)
		}

		// send event OnStartStanding (//???through callback?)
		_State = ECharacterState::Stand;
	}
	else if (DistanceToGround > MaxStepDownHeight && OnGround)
	{
		//???controll whole speed or only a vertical component? now the second

		// Y is inverted to be positive when the character moves downwards
		const float VerticalImpulse = _Body->GetMass() * -_Body->GetBtBody()->getLinearVelocity().y();
		if (VerticalImpulse > MaxLandingImpulse)
		{
			// send event OnEnd[state] (//???through callback?)
			// send event OnStartFalling (//???through callback?)
			_State = ECharacterState::Fall;
			_Body->SetActive(true);
		}
		else if (VerticalImpulse > 0.f)
		{
			// send event OnEnd[state] (//???through callback?)
			// send event OnStartJumping (//???through callback?)
			_State = ECharacterState::Jump;
			_Body->SetActive(true);
		}
		//???else if VerticalImpulse == 0.f levitate?
	}

	// Update the controller in the current state, including movement requests

	if (_State == ECharacterState::Stand)
	{
		if (_LinearMovementState == EMovementState::Requested)
		{
			//// Check if movement type is not available to the actor
			//!!!instead of movement types character controller must know its current speed, set externally with
			// the movement request. So the speed is not an indicator, it is always valid for the valid request.
			// Movement must be reset externally if the character lost an ability to move.
			// May also add explicit 'paralyzed' flag here, but it must not reset the request immediately, it is better
			// to wait until paralyzation is over and executing the request!
			//if (MaxSpeed[pActor->MvmtType] <= 0.f)
			//	ResetMovement(false);
			//???where to test movement caps? are tem set from external systems? if so, can store inside and react immediately,
			// without a per-tick check.
			//like: if new dest & _can't move_, set AIMvmt_Failed

			_DesiredLinearVelocity = CalcDesiredLinearVelocity(Pos);
			//!!!Can add separation with neighbours here!
			if (_ObstacleAvoidanceEnabled) AvoidObstacles();
			//if (_NeedDestinationFacing) RequestFacing(vector3(DesiredDir.x, 0.f, DesiredDir.y));
		}

		if (_AngularMovementState == EMovementState::Requested)
		{
			const float Angle = vector3::Angle2DNorm(LookatDir, _RequestedLookat);
			_DesiredAngularVelocity = CalcDesiredAngularVelocity(Angle);

			// Amount of required rotation is too big, actor must stop moving to perform it
			if (std::fabsf(Angle) > _BigTurnThreshold) _DesiredLinearVelocity = vector3::Zero;
		}

		// We want a precise control over the movement, so deny freezing on low speed
		// when movement is requested. When idle, allow to deactivate eventually.
		if (IsMotionRequested())
			_Body->SetActive(true, true);
		else if (_Body->IsAlwaysActive())
			_Body->SetActive(true, false);

		// No angular acceleration limit, set directly
		_Body->GetBtBody()->setAngularVelocity(btVector3(0.f, _DesiredAngularVelocity, 0.f));

		const float InvTickTime = 1.f / dt;

		//???what to do with requested y? perform auto climbing/jumping or deny and wait for an explicit command?
		btVector3 ReqLVel = btVector3(_DesiredLinearVelocity.x, 0.f, _DesiredLinearVelocity.z);
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
	n_assert_dbg(_Body && _Body->GetLevel() == pLevel);

	/*
	vector3 OldPos = Position;
	quaternion Rot;
	pCC->GetController()->GetBody()->Physics::CPhysicsObject::GetTransform(Position, Rot); //!!!need nonvirtual method "GetWorld/PhysicsTransform"!

	LookatDir = Rot.rotate(vector3::BaseDir);

	if (OldPos != Position)
	{
		constexpr float AngularTolerance = 0.00001f;
		if (AngleAbs < AngularTolerance) ResetRotation(true);
	
		if (pActor->MvmtState == AIMvmt_DestSet && pActor->IsAtPoint(DestPoint))
				ResetMovement(true);

		// Check if actor reached or crossed the destination last frame (with some tolerance).
		// For that we detect point on the last frame movement segment that is the closest to the destination
		// and check distance (in XZ, + height difference to handle possible navmesh stages).
		//!!!TEST is still useful after steering fixes?
		//if (pActor->MvmtState == AIMvmt_DestSet)
		//{
		//	vector3 LinVel;
		//	if (pActor->GetEntity()->GetAttr(LinVel, CStrID("LinearVelocity")))
		//	{
		//		vector3 FrameMovement = LinVel * FrameTime;
		//		vector3 PrevPos = pActor->Position - FrameMovement;
		//		float t = (FrameMovement.Dot(DestPoint) - FrameMovement.Dot(PrevPos)) / FrameMovement.SqLength();
		//		if (t >= 0.f && t <= 1.f)
		//		{
		//			vector3 Closest = PrevPos + FrameMovement * t;
		//			const float Tolerance = pActor->LinearArrivalTolerance * pActor->LinearArrivalTolerance;
		//			if (vector3::SqDistance2D(Closest, DestPoint) < Tolerance && n_fabs(Closest.y - DestPoint.y) < pActor->Height)
		//			{
		//				ResetMovement(true);
		//			}
		//		}
		//	}
		//}

		// If is stuck, increase stuck timer, and if timer is too high, set Stuck mvmt state, i.e. fail movement.
		// Maybe this must happen after the tick!
	}
	*/
}
//---------------------------------------------------------------------

// Calculate the distance to the nearest object under feet
float CCharacterController::CalcDistanceToGround(const vector3& Pos) const
{
	constexpr float GroundProbeLength = 0.5f;

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
		return Pos.y - ContactPos.y;
	}

	return std::numeric_limits<float>().max();
}
//---------------------------------------------------------------------

vector3 CCharacterController::CalcDesiredLinearVelocity(const vector3& Pos) const
{
	float Speed = _DesiredLinearSpeed;

	const float DistanceToRequestedPosition = vector3::Distance2D(Pos, _RequestedPosition);

	// Calculate arrival slowdown if close enough to destination
	// Negative _ArriveAdditionalDistance means that no arrive steering required at all
	if (_AdditionalArriveDistance >= 0.f)
	{
		// _AdditionalArriveDistance > 0 enables correct arrival when the current destination is not the final one.
		// Navigation system sets _AdditionalArriveDistance to the distance from the requested position to the
		// final destination, and here we calculate effective distance to the destination along the path without
		// knowledge about the path topology.
		const float Distance = DistanceToRequestedPosition + _AdditionalArriveDistance;

		// S = -v0^2/2a for 0 = v0 + at (stop condition)
		const float SlowDownRadius = _ArriveBrakingCoeff * Speed * Speed;
		if (Distance < SlowDownRadius)
			Speed *= ((2.f * SlowDownRadius - Distance) * Distance) / (SlowDownRadius * SlowDownRadius);
	}

	//!!!FIXME: what if final direction will not head to the destination?
	// Seek overshoot will possibly happen next frame, clamp speed. Overshoot is still possible
	// if frame rate is variable, but physics system uses fixed step.
	const float FrameTime = _Body->GetLevel()->GetStepTime();
	if (DistanceToRequestedPosition < Speed * FrameTime) Speed = DistanceToRequestedPosition / FrameTime;

	vector3 Velocity(_RequestedPosition.x - Pos.x, 0.f, _RequestedPosition.z - Pos.z);

	const float DirLength = Velocity.Length2D();

	// If current destination is an intermediate turning point, make the trajectory smooth
	if (_SteeringSmoothness > 0.f && _NextRequestedPosition != _RequestedPosition)
	{
		const vector3 ToNext(_NextRequestedPosition.x - Pos.x, 0.f, _NextRequestedPosition.z - Pos.z);
		const float DistToNext = ToNext.Length2D();
		if (DistToNext > 0.001f)
		{
			const float Scale = DirLength * _SteeringSmoothness / DistToNext;
			Velocity -= ToNext * Scale;
		}
	}

	if (DirLength > std::numeric_limits<float>().epsilon())
		Velocity *= Speed / DirLength;

	return Velocity;
}
//---------------------------------------------------------------------

float CCharacterController::CalcDesiredAngularVelocity(float Angle) const
{
	constexpr float AngularArriveZone = 0.34906585039886591538473815369772f; // 20 degrees in radians

	const bool IsNegative = (Angle < 0.f);
	const float AngleAbs = IsNegative ? -Angle : Angle;

	float Speed = _DesiredAngularSpeed;

	// Arrive slowdown
	if (AngleAbs <= AngularArriveZone)
		Speed *= AngleAbs / AngularArriveZone;

	// Avoid overshooting
	const float FrameTime = _Body->GetLevel()->GetStepTime();
	if (AngleAbs < Speed * FrameTime) Speed = AngleAbs / FrameTime;

	return IsNegative ? -_DesiredAngularSpeed : _DesiredAngularSpeed;
}
//---------------------------------------------------------------------

//!!!FIXME: what if obstacle is over the destination point? brake and stand stuck until request is failed or location if freed?
void CCharacterController::AvoidObstacles()
{
	/*
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
	
	*/
}
//---------------------------------------------------------------------

/*

void CMotorSystem::ResetMovement(bool Success)
{
	pActor->MvmtState = Success ? AIMvmt_Done : AIMvmt_Failed;
	Data::PParams PLinearVel = n_new(Data::CParams(1));
	PLinearVel->Set(CStrID("Velocity"), vector3::Zero);
	pActor->GetEntity()->FireEvent(CStrID("RequestLinearV"), PLinearVel);
}
//---------------------------------------------------------------------

void CMotorSystem::ResetRotation(bool Success)
{
	pActor->FacingState = Success ? AIFacing_Done : AIFacing_Failed;
	Data::PParams PAngularVel = n_new(Data::CParams(1));
	PAngularVel->Set(CStrID("Velocity"), 0.f);
	pActor->GetEntity()->FireEvent(CStrID("RequestAngularV"), PAngularVel);
}
//---------------------------------------------------------------------

//!!!WRITE IT!
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

void CMotorSystem::SetDest(const vector3& Dest)
{
	if (pActor->IsAtPoint(Dest)) ResetMovement(true);
	else
	{
		DestPoint = Dest;
		pActor->MvmtState = AIMvmt_DestSet;
		FaceDest = vector3::SqDistance2D(pActor->Position, DestPoint) > SqShortStepThreshold;
		//pLastClosestObstacle = nullptr;
	}
}
//---------------------------------------------------------------------

void CMotorSystem::SetFaceDirection(const vector3& Dir)
{
	if (pActor->IsLookingAtDir(Dir)) pActor->FacingState = AIFacing_Done;
	else
	{
		FaceDir = Dir;
		pActor->FacingState = AIFacing_DirSet;
	}
}
//---------------------------------------------------------------------

float CMotorSystem::GetMaxSpeed() const
{
	return std::max(MaxSpeed[pActor->MvmtType], 0.f);
}
//---------------------------------------------------------------------

void CMotorSystem::RenderDebug(Debug::CDebugDraw& DebugDraw)
{
	static const vector4 ColorNormal(1.0f, 1.0f, 1.0f, 1.0f);
	static const vector4 ColorStuck(1.0f, 0.0f, 0.0f, 1.0f);

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
