#include "ActionGotoSmartObj.h"

#include <AI/PropActorBrain.h>
#include <AI/PropSmartObject.h>
#include <Game/EntityManager.h>
#include <Math/Math.h>

namespace AI
{
__ImplementClass(AI::CActionGotoSmartObj, 'AGSO', AI::CActionGoto)

bool CActionGotoSmartObj::Activate(CActor* pActor)
{
	Game::CEntity* pEnt = EntityMgr->GetEntity(TargetID);
	if (!pEnt) FAIL;
	CPropSmartObject* pSO = pEnt->GetProperty<CPropSmartObject>();
	if (!pSO || !pSO->IsActionAvailable(ActionID, pActor)) FAIL;

	IsFacing = false;

	return UpdateNavDest(pActor, pSO);
}
//---------------------------------------------------------------------

DWORD CActionGotoSmartObj::Update(CActor* pActor)
{
	Game::CEntity* pEnt = EntityMgr->GetEntity(TargetID);
	if (!pEnt) return Failure;
	CPropSmartObject* pSO = pEnt->GetProperty<CPropSmartObject>();
	if (!pSO || !pSO->IsActionAvailable(ActionID, pActor)) FAIL; //???IsActionAvailable() - some interval instead of every frame check?

	if (pSO->IsMovable() && !UpdateNavDest(pActor, pSO)) return Failure;

	if (pActor->NavState == AINav_Done)
	{
		if (!IsFacing || pSO->IsMovable())
		{
			vector3 FaceDir;
			if (!pSO->GetRequiredActorFacing(ActionID, pActor, FaceDir)) return Success;
			pActor->GetMotorSystem().SetFaceDirection(FaceDir);
			IsFacing = true;
		}

		switch (pActor->FacingState)
		{
			case AIFacing_DirSet:	return Running;
			case AIFacing_Done:		return Success;
			case AIFacing_Failed:	return Failure;
			default: n_error("CActionGotoSmartObj::Update(): Unexpected facing status '%d'", pActor->FacingState);
		}
	}
	else
	{
		if (IsFacing)
		{
			if (pActor->FacingState == AIFacing_DirSet)
				pActor->GetMotorSystem().ResetRotation(false);
			IsFacing = false;
		}

		switch (pActor->NavState)
		{
			case AINav_Failed:		return Failure;
			case AINav_DestSet:		return Running;
			case AINav_Planning:
			case AINav_Following:	return AdvancePath(pActor);
			default: n_error("CActionGotoSmartObj::Update(): Unexpected navigation status '%d'", pActor->NavState);
		}
	}

	return Failure;
}
//---------------------------------------------------------------------

void CActionGotoSmartObj::Deactivate(CActor* pActor)
{
	if (pActor->FacingState == AIFacing_DirSet)
		pActor->GetMotorSystem().ResetRotation(false);
	CActionGoto::Deactivate(pActor);
}
//---------------------------------------------------------------------

//???update only if SO position changes?
bool CActionGotoSmartObj::UpdateNavDest(CActor* pActor, Prop::CPropSmartObject* pSO)
{
	if (pSO->IsActionAvailableFrom(ActionID, pActor->Position))
	{
		pActor->GetNavSystem().Reset(true);
		//!!!reset failed dest timer! also reset in other places where needed!
		OK;
	}

	//!!!if failed dest timer is over, fail as target is lost!
	//!!!threshold value must be stored in pActor and may be changed in runtime!

	const matrix44& Tfm = pSO->GetEntity()->GetAttr<matrix44>(CStrID("Transform"));
	vector3 SOPos = Tfm.Translation();

	if (pSO->IsMovable()) // && pActor->TargetPosPredictionMode != None
	{
		vector3 SOVelocity;
		if (pSO->GetEntity()->GetAttr<vector3>(SOVelocity, CStrID("LinearVelocity")) && SOVelocity.SqLength2D() > 0.f)
		{
			// Predict future SO position (No prediction, Pursue steering, Quadratic firing solution)

			vector3 Dist = SOPos - pActor->Position;
			float MaxSpeed = pActor->GetMotorSystem().GetMaxSpeed();
			float Time;

			if (true) // pActor->TargetPosPredictionMode == Quadratic
			{
				// Quadratic firing solution
				float A = SOVelocity.SqLength2D() - MaxSpeed * MaxSpeed;
				float B = 2.f * (Dist.x * SOVelocity.x + Dist.z * SOVelocity.z);
				float C = Dist.SqLength2D();
				float Time1, Time2;

				DWORD RootCount = Math::SolveQuadraticEquation(A, B, C, &Time1, &Time2);

				if (!RootCount) FAIL; //!!!keep a prev point some time! or use current pos?

				Time = (RootCount == 1 || (Time1 < Time2 && Time1 > 0.f)) ? Time1 : Time2;

				//???is possible?
				if (Time < 0.f) FAIL; //!!!keep a prev point some time! or use current pos?
			}
			else
			{
				// Pursue steering
				Time = Dist.Length2D() / MaxSpeed;
			}

			//???need? see if it adds realism!
			//const float MaxPredictionTime = 5.f; //???to settings?
			//if (Time > MaxPredictionTime) Time = MaxPredictionTime;

			SOPos.x += SOVelocity.x * Time;
			SOPos.z += SOVelocity.z * Time;

			//!!!assumed that a moving SO with a relatively high speed (no short step!) tries
			//to face to where it moves, predict facing smth like this:
			// get angle between curr facing and velocity direction
			// get maximum angle as Time * MaxAngularSpeed
			// get min(angle_between, max_angle) and rotate curr direction towards a velocity direction
			// this will be a predicted facing
			// may be essential for AI-made backstabs
		}
	}

	vector3 Dest;
	if (!pSO->GetRequiredActorPosition(ActionID, pActor, SOPos, Dest)) FAIL; //!!!keep a prev point some time! or fail?

	pActor->GetNavSystem().SetDestPoint(Dest);
	//!!!reset failed dest timer! also reset in other places where needed!

	OK;
}
//---------------------------------------------------------------------

}