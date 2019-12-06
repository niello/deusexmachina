#include "MotorSystem.h"

#include <AI/AIServer.h>
#include <AI/PropActorBrain.h>
#include <AI/Movement/Memory/MemFactObstacle.h>
#include <Debug/DebugDraw.h>
#include <DetourObstacleAvoidance.h>

#define OBSTACTLE_DETECTOR_MIN		0.1f
#define	OBSTACLE_PREDICTION_TIME	1.2f

namespace AI
{

void CMotorSystem::Init(const Data::CParams* Params)
{
	MaxSpeed[AIMvmt_Type_None] = 0.f;

	if (Params)
	{
		MaxSpeed[AIMvmt_Type_Walk] = Params->Get<float>(CStrID("MaxSpeedWalk"), 2.5f);
		MaxSpeed[AIMvmt_Type_Run] = Params->Get<float>(CStrID("MaxSpeedRun"), 6.f);
		MaxSpeed[AIMvmt_Type_Crouch] = Params->Get<float>(CStrID("MaxSpeedCrouch"), 1.f);
		MaxAngularSpeed = Params->Get<float>(CStrID("MaxAngularSpeed"), PI);
		ArriveCoeff = -0.5f / Params->Get<float>(CStrID("MaxBrakingAccel"), -5.f);
	}
	else
	{
		MaxSpeed[AIMvmt_Type_Walk] = 2.5f;
		MaxSpeed[AIMvmt_Type_Run] = 6.f;
		MaxSpeed[AIMvmt_Type_Crouch] = 1.f;
		MaxAngularSpeed = PI;
		ArriveCoeff = -0.5f / -10.f;
	}
}
//---------------------------------------------------------------------

//!!!update per physics tick!
void CMotorSystem::Update(float FrameTime)
{
	//???where to test movement caps? here, read from BB?
	//like: if new dest & _can't move_, set AIMvmt_Failed

	vector3 LinearVel;

	// Update stuck info
	if (pActor->MvmtState == AIMvmt_DestSet || pActor->MvmtState == AIMvmt_Stuck)
	{
		if (IsStuck())
		{
			if (pActor->MvmtState = AIMvmt_DestSet)
			{
				pActor->MvmtState = AIMvmt_Stuck;
				//StuckTime = 0.f; //or StuckTime = CurrTime;
			}
			//else StuckTime += FrameTime; //???!!!if we store relative time, not state entering time
			//if (StuckTime > StuckForTooLongTime) ResetMovement(false);
		}
		else pActor->MvmtState = AIMvmt_DestSet;
	}

	// Check if actor reached or crossed the destination last frame (with some tolerance).
	// For that we detect point on the last frame movement segment that is the closest to the destination
	// and check distance (in XZ, + height difference to handle possible navmesh stages).
	//!!!TEST is still useful after steering fixes?
	if (pActor->MvmtState == AIMvmt_DestSet)
	{
		vector3 LinVel;
		if (pActor->GetEntity()->GetAttr(LinVel, CStrID("LinearVelocity")))
		{
			vector3 FrameMovement = LinVel * FrameTime;
			vector3 PrevPos = pActor->Position - FrameMovement;
			float t = (FrameMovement.Dot(DestPoint) - FrameMovement.Dot(PrevPos)) / FrameMovement.SqLength();
			if (t >= 0.f && t <= 1.f)
			{
				vector3 Closest = PrevPos + FrameMovement * t;
				const float Tolerance = pActor->LinearArrivalTolerance * pActor->LinearArrivalTolerance;
				if (vector3::SqDistance2D(Closest, DestPoint) < Tolerance && n_fabs(Closest.y - DestPoint.y) < pActor->Height)
				{
					ResetMovement(true);
				}
			}
		}
	}

	// Check if movement type is not available to the actor
	if (pActor->MvmtState == AIMvmt_DestSet && MaxSpeed[pActor->MvmtType] <= 0.f)
		ResetMovement(false);

	if (pActor->MvmtState == AIMvmt_DestSet)
	{
		float Speed = MaxSpeed[pActor->MvmtType];

		const float SlowDownRadius = ArriveCoeff * Speed * Speed; // S = -v0^2/2a for 0 = v0 + at (stop condition)

		//!!!AISteer_Type_Arrive - set somewhere!

		// Big turn detected or the next traversal action requires stop
		float LocalArrive = 1.f;
		float LocalDist = vector3::Distance2D(pActor->Position, DestPoint);
		if (pActor->SteeringType == AISteer_Type_Arrive && LocalDist < SlowDownRadius)
			LocalArrive = ((2.f * SlowDownRadius - LocalDist) * LocalDist) / (SlowDownRadius * SlowDownRadius);

		// Path target is near
		float GlobalArrive = (pActor->DistanceToNavDest < SlowDownRadius) ?
			((2.f * SlowDownRadius - pActor->DistanceToNavDest) * pActor->DistanceToNavDest) / (SlowDownRadius * SlowDownRadius) :
			1.f;

		Speed *= std::min(LocalArrive, GlobalArrive);

		// Seek overshoot will possibly happen next frame, clamp speed. Overshoot is still possible
		// if frame rate is variable, so abowe we detect if actor crossed destination last frame.
		if (LocalDist < Speed * FrameTime) Speed = LocalDist / FrameTime;

		vector2 DesiredDir(DestPoint.x - pActor->Position.x, DestPoint.z - pActor->Position.z);

		if (SmoothSteering && DestPoint != NextDestPoint)
		{
			vector2 ToNext(NextDestPoint.x - pActor->Position.x, NextDestPoint.z - pActor->Position.z);

			const float SmoothnessCoeff = 0.3f; //!!!to settings! [0 to 1), [0 - direct, 1) - big curve
			float Scale = DesiredDir.Length() * SmoothnessCoeff;
			float DistToNext = ToNext.Length();
			if (DistToNext > 0.001f) Scale /= DistToNext;

			DesiredDir -= ToNext * Scale;
		}

		DesiredDir.norm();

		//!!!Separation with neighbours here:

		LinearVel.set(DesiredDir.x * Speed, 0.f, DesiredDir.y * Speed);

		if (AvoidObstacles)
		{
#ifdef DETOUR_OBSTACLE_AVOIDANCE // In ActorFwd.h
			float Range = pActor->Radius + OBSTACTLE_DETECTOR_MIN + Speed * OBSTACLE_PREDICTION_TIME;

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

				float DetectorLength = ActorRadius + OBSTACTLE_DETECTOR_MIN + Speed * OBSTACLE_PREDICTION_TIME;

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

		if (FaceDest) SetFaceDirection(vector3(DesiredDir.x, 0.f, DesiredDir.y));
	}

	if (pActor->FacingState == AIFacing_DirSet)
	{
		float Angle = vector3::Angle2DNorm(pActor->LookatDir, FaceDir);
		float AngleAbs = n_fabs(Angle);

		if (AngleAbs < pActor->AngularArrivalTolerance) ResetRotation(true);
		else
		{
			if (MaxAngularSpeed <= 0.f) ResetRotation(false); // We can't rotate
			else
			{
				if (AngleAbs > BigTurnThreshold) LinearVel = vector4::Zero;

				// Start arrive slowdown at 20 degrees to goal
				float AngularVel = (Angle < 0) ? -MaxAngularSpeed : MaxAngularSpeed;
				//???clamp to Angle / FrameTime to avoid overshoots for high speeds?
				if (AngleAbs <= 0.34906585039886591538473815369772f) // 20 deg in rads
					AngularVel *= AngleAbs * 2.8647889756541160438399077407053f; // 1 / (20 deg in rads)

				Data::PParams PAngularVel = n_new(Data::CParams(1));
				PAngularVel->Set(CStrID("Velocity"), AngularVel);
				pActor->GetEntity()->FireEvent(CStrID("RequestAngularV"), PAngularVel);
			}
		}
	}

	// Delayed for a big turn check
	if (pActor->MvmtState == AIMvmt_DestSet)
	{
		Data::PParams PLinearVel = n_new(Data::CParams(1));
		PLinearVel->Set(CStrID("Velocity"), LinearVel);
		pActor->GetEntity()->FireEvent(CStrID("RequestLinearV"), PLinearVel);
	}
}
//---------------------------------------------------------------------

void CMotorSystem::UpdatePosition()
{
	if (pActor->MvmtState == AIMvmt_DestSet && pActor->IsAtPoint(DestPoint))
		ResetMovement(true);
}
//---------------------------------------------------------------------

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

void CMotorSystem::RenderDebug()
{
	static const vector4 ColorNormal(1.0f, 1.0f, 1.0f, 1.0f);
	static const vector4 ColorStuck(1.0f, 0.0f, 0.0f, 1.0f);

	if (pActor->MvmtState == AIMvmt_DestSet || pActor->MvmtState == AIMvmt_Stuck)
		DebugDraw->DrawLine(
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
		DebugDraw->DrawCylinder(Tfm, pObstacle->Radius, 1.f, Color); // pObstacle->Height instead of 1.f
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

	DebugDraw->DrawText(Text.CStr(), 0.05f, 0.1f);
}
//---------------------------------------------------------------------

}