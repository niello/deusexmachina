#include "ActionGotoTarget.h"

#include <AI/PropActorBrain.h>
#include <Game/GameServer.h>
#include <Core/Factory.h>

namespace AI
{
__ImplementClass(AI::CActionGotoTarget, 'AGTG', AI::CActionGoto)

bool CActionGotoTarget::Activate(CActor* pActor)
{
	Game::CEntity* pEnt = GameSrv->GetEntityMgr()->GetEntity(TargetID);
	if (!pEnt) FAIL;
	pActor->GetNavSystem().SetDestPoint(pEnt->GetAttr<matrix44>(CStrID("Transform")).Translation());

	//!!!Get IsDynamic as (BB->WantToFollow && IsTargetMovable)!
	IsDynamic = false;

	OK;
}
//---------------------------------------------------------------------

UPTR CActionGotoTarget::Update(CActor* pActor)
{
	//!!!can use intercept instead of pursue! see goto SO, extract
	//prediction from CPropSmartObject::GetRequiredActorPosition()
	if (IsDynamic && !pActor->IsNavSystemIdle())
	{
		Game::CEntity* pEnt = GameSrv->GetEntityMgr()->GetEntity(TargetID);
		if (!pEnt) return Failure;
		pActor->GetNavSystem().SetDestPoint(pEnt->GetAttr<matrix44>(CStrID("Transform")).Translation());
		if (pActor->NavState == AINav_Done) return Success;
	}

	return CActionGoto::Update(pActor);
}
//---------------------------------------------------------------------

}