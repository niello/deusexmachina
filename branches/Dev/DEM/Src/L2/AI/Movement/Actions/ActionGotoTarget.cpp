#include "ActionGotoTarget.h"

#include <AI/Prop/PropActorBrain.h>
#include <Game/EntityManager.h>

namespace AI
{
__ImplementClass(AI::CActionGotoTarget, 'AGTG', AI::CActionGoto)

bool CActionGotoTarget::Activate(CActor* pActor)
{
	Game::CEntity* pEnt = EntityMgr->GetEntity(TargetID);
	if (!pEnt) FAIL;

	pActor->GetNavSystem().SetDestPoint(pEnt->GetAttr<matrix44>(CStrID("Transform")).Translation());

	//!!!Get IsDynamic as (BB->WantToFollow && IsTargetMovable)!
	IsDynamic = false;

	//CActionGoto::Activate(pActor)
	OK;
}
//---------------------------------------------------------------------

EExecStatus CActionGotoTarget::Update(CActor* pActor)
{
	switch (pActor->NavStatus)
	{
		case AINav_Invalid:
		case AINav_Failed:		return Failure;
		case AINav_Done:		return Success;
		case AINav_DestSet:		return Running;
		case AINav_Planning:
		{
			// Can follow temporary path
			return Running;
		}
		case AINav_Following:
		{
			if (IsDynamic)
			{
				Game::CEntity* pEnt = EntityMgr->GetEntity(TargetID);
				if (!pEnt) return Failure;
				pActor->GetNavSystem().SetDestPoint(pEnt->GetAttr<matrix44>(CStrID("Transform")).Translation());
			}
			return AdvancePath(pActor);
		}
		default: n_error("CActionGotoTarget::Update(): Unexpected navigation status '%d'", pActor->NavStatus);
	}

	return Failure;
}
//---------------------------------------------------------------------

} //namespace AI