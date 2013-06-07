#include "MotorSystem.h"

#include <AI/AIServer.h>
#include <AI/Prop/PropActorBrain.h>
#include <AI/Movement/Memory/MemFactObstacle.h>
#include <Render/DebugDraw.h>
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
	}
	else
	{
		MaxSpeed[AIMvmt_Type_Walk] = 2.5f;
		MaxSpeed[AIMvmt_Type_Run] = 6.f;
		MaxSpeed[AIMvmt_Type_Crouch] = 1.f;
		MaxAngularSpeed = PI;
	}
}
//---------------------------------------------------------------------

void CMotorSystem::Update()
{
	//???where to test movement caps? here, read from BB?
	//like: if new dest & can't move, set AIMvmt_CantMove/AIMvmt_Failed

	vector4 LinearVel;

	switch (pActor->MvmtStatus)
	{
		//???need both states?
		case AIMvmt_None:
		case AIMvmt_Done:
		{
			break;
		}
		case AIMvmt_Stuck:
		{
			if (IsStuck())
			{
				//StuckTime += FrameTime; //if we store relative time, not state entering time
				//if (StuckTime > StuckForTooLongTime)
				//{
				//	ResetMovement();
				//	pActor->MvmtStatus = AIMvmt_None;	//!!!???as argument to ResetMovement?! like bool Success?
				//										//???need both AIMvmt_None & AIMvmt_Done at all?
				//	break;
				//}
				//break;
			}
			else pActor->MvmtStatus = AIMvmt_DestSet;
			break;
		}
		case AIMvmt_DestSet:
		{
			if (pActor->IsAtPoint(DestPoint, false))
			{
				ResetMovement();
				break;
			}

			if (IsStuck())
			{
				pActor->MvmtStatus = AIMvmt_Stuck;
				//StuckTime = 0.f; //or StuckTime = CurrTime;
				break;
			}

			float Speed = MaxSpeed[pActor->MvmtType];

			// Movement type is unavailable to the actor
			if (Speed <= 0.f)
			{
				ResetMovement();
				pActor->MvmtStatus = AIMvmt_None;	//!!!???as argument to ResetMovement?! like bool Success?
													//???need both AIMvmt_None & AIMvmt_Done at all?
				break;
			}

			if (pActor->SteeringType == AISteer_Type_Arrive)
			{
				float DistToNavDest = pActor->DistanceToNavDest -
					(pActor->DistanceToNavDest > pActor->MaxReachDist ? pActor->MaxReachDist : pActor->MinReachDist);

				const float SlowDownRadius = Speed * 0.5f; // Distance actor moves by at 0.5 sec

				if (DistToNavDest < SlowDownRadius)
					Speed *= ((2.f * SlowDownRadius - DistToNavDest) * DistToNavDest) / (SlowDownRadius * SlowDownRadius);
			}

			vector2 DesiredDir(DestPoint.x - pActor->Position.x, DestPoint.z - pActor->Position.z);

			if (SmoothSteering && DestPoint != NextDestPoint)
			{
				vector2 ToNext(NextDestPoint.x - pActor->Position.x, NextDestPoint.z - pActor->Position.z);

				float Scale = DesiredDir.len() * 0.5f;
				float DistToNext = ToNext.len();
				if (DistToNext > 0.001f) Scale /= DistToNext;

				DesiredDir -= ToNext * Scale;
			}

			DesiredDir.norm();

			// Separation with neighbours here:
			//!!!

			// y & w are 0.f as set in vector4 constructor
			LinearVel.x = DesiredDir.x * Speed;
			LinearVel.z = DesiredDir.y * Speed;

			//!!!take obstacle into account only if it is close enough!
			if (AvoidObstacles)
			{
//#define DETOUR_OBSTACLE_AVOIDANCE
#ifdef DETOUR_OBSTACLE_AVOIDANCE
				float Range = pActor->Radius + OBSTACTLE_DETECTOR_MIN + Speed * OBSTACLE_PREDICTION_TIME;

				//???here or global persistent?
				dtObstacleAvoidanceQuery ObstacleQuery;
				ObstacleQuery.init(6, 8);
				//ObstacleQuery.reset();
				pActor->GetNavSystem().GetObstacles(Range, ObstacleQuery);

				//???pre-filter mem facts? distance < Range, height intersects actor height etc
				CMemFactNode* pCurr = pActor->GetMemSystem().GetFactsByType(CMemFactObstacle::RTTI);
				for (; pCurr; pCurr = pCurr->GetSucc())
				{
					CMemFactObstacle* pObstacle = (CMemFactObstacle*)pCurr->Object.CStr();
					//!!!remember obstacle velocity in the fact and use here!
					// desired velocity can be get from obstacles-actors only!
					ObstacleQuery.addCircle(pObstacle->Position.v, pObstacle->Radius, vector3::Zero.v, vector3::Zero.v);
				}

				//!!!GET AT ACTIVATION AND STORE!
				const dtObstacleAvoidanceParams* pOAParams = AISrv->GetDefaultObstacleAvoidanceParams();

				if (ObstacleQuery.getObstacleCircleCount() || ObstacleQuery.getObstacleSegmentCount())
				{
					//!!!???track LinearVelocity in actor like position?
					Prop::CPropActorPhysics* pPhysics = pActor->GetEntity()->GetProperty<Prop::CPropActorPhysics>();
					const vector3& Velocity = pPhysics ? pPhysics->GetPhysicsEntity()->GetVelocity() : vector3::Zero;

					float DesVel[3] = { LinearVel.x, 0.f, LinearVel.z }; // Copy LVel because it is modified inside the call
					if (AdaptiveVelocitySampling)
						ObstacleQuery.sampleVelocityAdaptive(pActor->Position.v, pActor->Radius, MaxSpeed[pActor->MvmtType],
															 Velocity.v, DesVel, LinearVel.v, pOAParams);
					else
						ObstacleQuery.sampleVelocityGrid(pActor->Position.v, pActor->Radius, MaxSpeed[pActor->MvmtType],
														 Velocity.v, DesVel, LinearVel.v, pOAParams);
				}

#else
				CMemFactNode* pCurr = pActor->GetMemSystem().GetFactsByType(CMemFactObstacle::RTTI);

				//!!!???dynamic obstacles - take velocity into account!?
				//!!!if get stuck avoiding obstacle, try to select far point

				if (pCurr)
				{
					CMemFactObstacle* pClosest = NULL;
					float MinIsect = FLT_MAX;
					float MinExpRadius;

					vector2 ActorPos(pActor->Position.x, pActor->Position.z);
					vector2 ToDest(DestPoint.x - ActorPos.x, DestPoint.z - ActorPos.y);
					float DistToDest = ToDest.len();
					ToDest /= DistToDest;
					vector2 ActorSide(-ToDest.y, ToDest.x);
					float ActorRadius = pActor->Radius + AvoidanceMargin;

					float DetectorLength = ActorRadius + OBSTACTLE_DETECTOR_MIN + Speed * OBSTACLE_PREDICTION_TIME;

					for (; pCurr; pCurr = pCurr->GetSucc())
					{
						CMemFactObstacle* pObstacle = (CMemFactObstacle*)pCurr->Object.Get();

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
						if (FromObstacleToDest.lensquared() < SqExpRadius) continue;

						vector2 ToObstacle(
							pObstacle->Position.x - ActorPos.x,
							pObstacle->Position.z - ActorPos.y);
						float SqDistToObstacle = ToObstacle.lensquared();

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
							float ProjDirLen = ProjDir.len();
							if (ProjDirLen <= TINY)
							{
								ProjDir.set(-ToObstacle.y, ToObstacle.x);
								ProjDirLen = ProjDir.len();
							}

							ToDest = ToObstacle + ProjDir * (MinExpRadius / ProjDirLen);
							ToDest.norm();
							LastAvoidDir = ToDest;
						}

						//!!!SPEED needs adjusting to! brake or arrival effect elimination.
						LinearVel.set(ToDest.x * Speed, 0.f, ToDest.y * Speed, 0.f);

						LastClosestObstaclePos = ObstaclePos;
					}

					pLastClosestObstacle = pClosest;
				}
#endif
			}

			if (FaceDest) SetFaceDirection(vector3(DesiredDir.x, 0.f, DesiredDir.y));

			break;
		}
		default: n_error("CMotorSystem::Update(): Unexpected movement status '%d'", pActor->MvmtStatus);
	}

	if (pActor->FacingStatus == AIFacing_DirSet)
	{
		// Since facing is performed in XZ plane, we simplify our calculations.
		// Originally SinA = CrossY = (CurrLookat x FaceDir) * LookatPlaneNormal
		// and CosA = Dot = CurrLookat * FaceDir
		// LookatPlaneNormal = Up = (0, 1, 0), so we need only Y component of Cross.

		float CrossY = pActor->LookatDir.z * FaceDir.x - pActor->LookatDir.x * FaceDir.z;
		float Dot = pActor->LookatDir.x * FaceDir.x + pActor->LookatDir.z * FaceDir.z;
		float Angle = atan2f(CrossY, Dot);
		float AngleAbs = n_fabs(Angle);

		float AngularVel;

		// We want to turn
		if (AngleAbs > 0.005f)
		{
			if (AngleAbs > BigTurnThreshold) LinearVel = vector4::Zero;

			static const float InvAngularArrive = 1.f / n_deg2rad(20.f);
			AngularVel = MaxAngularSpeed * AngleAbs * InvAngularArrive;
			if (AngularVel > MaxAngularSpeed) AngularVel = MaxAngularSpeed;
			if (Angle < 0) AngularVel = -AngularVel;

			PParams PAngularVel = n_new(CParams);
			PAngularVel->Set(CStrID("Velocity"), AngularVel);
			pActor->GetEntity()->FireEvent(CStrID("RequestAngularV"), PAngularVel);
			WasFacingPrevFrame = true;
		}
		else
		{
			pActor->FacingStatus = AIFacing_Done;
			ResetRotation();
		}
	}
	else if (WasFacingPrevFrame) ResetRotation();

	//!!!CAN reset rotation if MaxAngularSpeed == 0.f!

	// Delayed for a big turn check
	if (pActor->MvmtStatus == AIMvmt_DestSet)
	{
		PParams PLinearVel = n_new(CParams);
		PLinearVel->Set(CStrID("Velocity"), LinearVel);
		pActor->GetEntity()->FireEvent(CStrID("RequestLinearV"), PLinearVel);
	}
}
//---------------------------------------------------------------------

void CMotorSystem::ResetMovement()
{
	pActor->MvmtStatus = AIMvmt_Done;
	PParams PLinearVel = n_new(CParams);
	PLinearVel->Set(CStrID("Velocity"), vector4::Zero);
	pActor->GetEntity()->FireEvent(CStrID("RequestLinearV"), PLinearVel);
}
//---------------------------------------------------------------------

void CMotorSystem::ResetRotation()
{
	if (pActor->FacingStatus == AIFacing_DirSet)
	{
		//!!!???Threshold!? 0.85f is TOO ROUGH!
		float Dot = pActor->LookatDir.x * FaceDir.x + pActor->LookatDir.z * FaceDir.z;
		pActor->FacingStatus = (Dot > 0.85f) ? AIFacing_Done : AIFacing_Failed;
	}
	PParams PAngularVel = n_new(CParams);
	PAngularVel->Set(CStrID("Velocity"), 0.f);
	pActor->GetEntity()->FireEvent(CStrID("RequestAngularV"), PAngularVel);
	WasFacingPrevFrame = false;
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

void CMotorSystem::RenderDebug()
{
	static const vector4 ColorNormal(1.0f, 1.0f, 1.0f, 1.0f);
	static const vector4 ColorStuck(1.0f, 0.0f, 0.0f, 1.0f);

	if (pActor->MvmtStatus == AIMvmt_DestSet || pActor->MvmtStatus == AIMvmt_Stuck)
		DebugDraw->DrawLine(
			vector3(DestPoint.x, pActor->Position.y, DestPoint.z),
			vector3(DestPoint.x, pActor->Position.y + 1.f, DestPoint.z),
			pActor->MvmtStatus == AIMvmt_DestSet ? ColorNormal : ColorStuck);

	CMemFactNode* pCurr = pActor->GetMemSystem().GetFactsByType(CMemFactObstacle::RTTI);
	for (; pCurr; pCurr = pCurr->GetSucc())
	{
		CMemFactObstacle* pObstacle = (CMemFactObstacle*)pCurr->Object.Get();
		matrix44 Tfm;
		Tfm.rotate_x(PI * 0.5f);
		Tfm.set_translation(pObstacle->Position);
		vector4 Color(0.6f, 0.f, 0.8f, 0.5f * pObstacle->Confidence);
		if (pObstacle == pLastClosestObstacle)
		{
			Color.x = 0.4f;
			Color.z = 0.9f;
		}
		DebugDraw->DrawCylinder(Tfm, pObstacle->Radius, 1.f, Color); // pObstacle->Height instead of 1.f
	}

	LPCSTR pMvmt = NULL;
	if (pActor->MvmtStatus == AIMvmt_None) pMvmt = "None";
	else if (pActor->MvmtStatus == AIMvmt_Done) pMvmt = "Done";
	else if (pActor->MvmtStatus == AIMvmt_DestSet) pMvmt = "DestSet";
	else if (pActor->MvmtStatus == AIMvmt_Stuck) pMvmt = "Stuck";

	nString text;
	text.Format("Mvmt status: %s\nFace direction set: %s\n", pMvmt, pActor->FacingStatus == AIFacing_DirSet ? "true" : "false");

	if (pActor->MvmtStatus == AIMvmt_DestSet)
	{
		vector2 ToDest(DestPoint.x - pActor->Position.x, DestPoint.z - pActor->Position.z);
		
		nString text2;
		text2.Format("DestPoint: %.4f, %.4f, %.4f\n"
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
			ToDest.len(),
			pActor->MinReachDist,
			pActor->MaxReachDist);
		text += text2;
	}

	DebugDraw->DrawText(text.CStr(), 0.05f, 0.1f);
}
//---------------------------------------------------------------------

void CMotorSystem::SetDest(const vector3& Dest)
{
	if (pActor->IsAtPoint(Dest, false)) ResetMovement();
	else
	{
		DestPoint = Dest;
		pActor->MvmtStatus = AIMvmt_DestSet;
		FaceDest = vector3::SqDistance2D(pActor->Position, DestPoint) > SqShortStepThreshold;
		//pLastClosestObstacle = NULL;
	}
}
//---------------------------------------------------------------------

void CMotorSystem::SetFaceDirection(const vector3& Dir)
{
	FaceDir = Dir;
	pActor->FacingStatus = AIFacing_DirSet;
}
//---------------------------------------------------------------------

} //namespace AI