#include "ActionGotoSmartObj.h"

#include <AI/Prop/PropActorBrain.h>
#include <AI/Prop/PropSmartObject.h>
#include <Game/EntityManager.h>

namespace Attr
{
	DeclareAttr(Transform);
}

namespace AI
{
__ImplementClass(AI::CActionGotoSmartObj, 'AGSO', AI::CActionGoto)

using namespace Properties;

bool CActionGotoSmartObj::Activate(CActor* pActor)
{
	Game::CEntity* pEnt = EntityMgr->GetEntity(TargetID);
	if (!pEnt) FAIL;

	CPropSmartObject* pSO = pEnt->GetProperty<CPropSmartObject>();
	n_assert(pSO);

	vector3 Dest;
	if (!pSO->GetDestination(ActionID, pActor->Radius, Dest, pActor->MinReachDist, pActor->MaxReachDist))
		FAIL;

	// Can modify pActor->MinReachDist and pActor->MaxReachDist here with ArrivalTolerance, if they are more.
	// Then actor will arrive exactly to the distance required.

	pActor->GetNavSystem().SetDestPoint(Dest);

	//return CActionGoto::Activate(pActor)

	OK;
}
//---------------------------------------------------------------------

} //namespace AI