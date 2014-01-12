#include "ActionGotoSmartObj.h"

#include <AI/PropActorBrain.h>
#include <AI/PropSmartObject.h>
#include <Game/EntityManager.h>

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

bool CActionGotoSmartObj::UpdateNavDest(CActor* pActor, Prop::CPropSmartObject* pSO)
{
	//???move code from the inside here entirely? get action by ID, get its destination params, calc destination
	vector3 DestOffset;
	if (!pSO->GetDestinationParams(ActionID, pActor, DestOffset, pActor->MinReachDist, pActor->MaxReachDist))
		FAIL;

	// Can modify pActor->MinReachDist and pActor->MaxReachDist here with ArrivalTolerance, if they are more.
	// Then actor will arrive exactly to the distance required.

	vector3 Dest = pSO->GetEntity()->GetAttr<matrix44>(CStrID("Transform")).Translation();

	// This is a pursue steering behaviour //???are pursue & intercept the same?
	if (pSO->IsMovable())
	{
		vector3 SOVelocity;
		if (pSO->GetEntity()->GetAttr<vector3>(SOVelocity, CStrID("LinearVelocity")) && SOVelocity.SqLength2D() > 0.f)
		{
			float DistToDest = vector3::Distance2D(pActor->Position, Dest);
			if (DistToDest > pActor->MaxReachDist || DistToDest < pActor->MinReachDist)
			{
				float MaxSpeed = pActor->GetMotorSystem().GetMaxSpeed();
				const float MaxPredictionTime = 5.f; //???to settings?
				float PredictionTime = (DistToDest >= MaxSpeed * MaxPredictionTime) ? MaxPredictionTime : DistToDest / MaxSpeed;
				SOVelocity.y = 0.f;
				Dest += SOVelocity * PredictionTime;
			}
		}
	}

	Dest += DestOffset;

	pActor->GetNavSystem().SetDestPoint(Dest);
	OK;
}
//---------------------------------------------------------------------

}