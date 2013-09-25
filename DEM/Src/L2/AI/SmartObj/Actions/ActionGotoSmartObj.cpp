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
	if (!pSO) FAIL;

	return UpdateDestination(pActor, pSO);
}
//---------------------------------------------------------------------

EExecStatus CActionGotoSmartObj::Update(CActor* pActor)
{
	Game::CEntity* pEnt = EntityMgr->GetEntity(TargetID);
	if (!pEnt) return Failure;
	CPropSmartObject* pSO = pEnt->GetProperty<CPropSmartObject>();
	if (!pSO) return Failure;

	if (pSO->IsMovable() && !pActor->IsNavSystemIdle() && !UpdateDestination(pActor, pSO)) return Failure;

	return CActionGoto::Update(pActor);
}
//---------------------------------------------------------------------

bool CActionGotoSmartObj::UpdateDestination(CActor* pActor, Prop::CPropSmartObject* pSO)
{
	//???some interval instead of every frame check?
	if (!pSO->GetAction(ActionID)->IsValid(pActor, pSO)) FAIL;

	//???move code from the inside here entirely? get action by ID, get its destination params, calc destination
	vector3 DestOffset;
	if (!pSO->GetDestinationParams(ActionID, pActor->Radius, DestOffset, pActor->MinReachDist, pActor->MaxReachDist))
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