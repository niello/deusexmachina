#include "ActionGotoSmartObj.h"

#include <AI/PropActorBrain.h>
#include <AI/PropSmartObject.h>
#include <Game/EntityManager.h>

namespace AI
{
__ImplementClass(AI::CActionGotoSmartObj, 'AGSO', AI::CActionGoto)

bool CActionGotoSmartObj::Activate(CActor* pActor)
{
	return UpdateDestination(pActor);
}
//---------------------------------------------------------------------

EExecStatus CActionGotoSmartObj::Update(CActor* pActor)
{
	if (IsSOMovable && !pActor->IsNavSystemIdle())
		if (!UpdateDestination(pActor)) return Failure;
	return CActionGoto::Update(pActor);
}
//---------------------------------------------------------------------

//!!!can use intercept instead of pursue!
bool CActionGotoSmartObj::UpdateDestination(CActor* pActor)
{
	Game::CEntity* pEnt = EntityMgr->GetEntity(TargetID);
	if (!pEnt) FAIL;
	CPropSmartObject* pSO = pEnt->GetProperty<CPropSmartObject>();
	if (!pSO) FAIL;

	//???some interval instead of every frame check?
	if (!pSO->GetAction(ActionID)->IsValid(pActor, pSO)) FAIL;

	vector3 DestOffset;
	if (!pSO->GetDestinationParams(ActionID, pActor->Radius, DestOffset, pActor->MinReachDist, pActor->MaxReachDist))
		FAIL;

	// Can modify pActor->MinReachDist and pActor->MaxReachDist here with ArrivalTolerance, if they are more.
	// Then actor will arrive exactly to the distance required.

	vector3 Dest = pEnt->GetAttr<matrix44>(CStrID("Transform")).Translation();

	// Can intercept here if SO is movable and has nonzero velocity

	Dest += DestOffset;

	pActor->GetNavSystem().SetDestPoint(Dest);
	IsSOMovable = pSO->IsMovable();
	OK;
}
//---------------------------------------------------------------------

}