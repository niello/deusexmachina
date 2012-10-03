#include "ActionGotoSmartObj.h"

#include <AI/Prop/PropActorBrain.h>
#include <AI/Prop/PropSmartObject.h>
#include <Game/Mgr/EntityManager.h>

namespace Attr
{
	DeclareAttr(Transform);
}

namespace AI
{
ImplementRTTI(AI::CActionGotoSmartObj, AI::CActionGoto)
ImplementFactory(AI::CActionGotoSmartObj);

using namespace Properties;

bool CActionGotoSmartObj::Activate(CActor* pActor)
{
	Game::CEntity* pEnt = EntityMgr->GetEntityByID(TargetID);
	if (!pEnt) FAIL;

	CPropSmartObject* pSO = pEnt->FindProperty<CPropSmartObject>();
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