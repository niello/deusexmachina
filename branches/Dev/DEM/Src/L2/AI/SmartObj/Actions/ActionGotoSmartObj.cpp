#include "ActionGotoSmartObj.h"

#include <AI/PropActorBrain.h>
#include <AI/PropSmartObject.h>
#include <Game/EntityManager.h>

namespace AI
{
__ImplementClass(AI::CActionGotoSmartObj, 'AGSO', AI::CActionGoto)

//???!!!in Updare when AINav_Done if target moved and we aren't in the dest, reactivate insterad of Success?!

bool CActionGotoSmartObj::Activate(CActor* pActor)
{
	Game::CEntity* pEnt = EntityMgr->GetEntity(TargetID);
	if (!pEnt) FAIL;
	CPropSmartObject* pSO = pEnt->GetProperty<CPropSmartObject>();
	if (!pSO) FAIL;

	vector3 Dest;
	if (!pSO->GetDestination(ActionID, pActor->Radius, Dest, pActor->MinReachDist, pActor->MaxReachDist))
		FAIL;

	// Can modify pActor->MinReachDist and pActor->MaxReachDist here with ArrivalTolerance, if they are more.
	// Then actor will arrive exactly to the distance required.

	pActor->GetNavSystem().SetDestPoint(Dest);

	OK;
}
//---------------------------------------------------------------------

}