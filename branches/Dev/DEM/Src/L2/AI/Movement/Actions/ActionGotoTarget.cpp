#include "ActionGotoTarget.h"

#include <AI/Prop/PropActorBrain.h>
#include <Game/Mgr/EntityManager.h>

namespace Attr
{
	DeclareAttr(Transform);
}

namespace AI
{
ImplementRTTI(AI::CActionGotoTarget, AI::CActionGoto)
ImplementFactory(AI::CActionGotoTarget);

bool CActionGotoTarget::Activate(CActor* pActor)
{
	Game::CEntity* pEnt = EntityMgr->GetEntityByID(TargetID);
	if (!pEnt) FAIL;

	pActor->GetNavSystem().SetDestPoint(pEnt->Get<matrix44>(Attr::Transform).pos_component());

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
				Game::CEntity* pEnt = EntityMgr->GetEntityByID(TargetID);
				if (!pEnt) return Failure;
				pActor->GetNavSystem().SetDestPoint(pEnt->Get<matrix44>(Attr::Transform).pos_component());
			}
			return AdvancePath(pActor);
		}
		default: n_error("CActionGotoTarget::Update(): Unexpected navigation status '%d'", pActor->NavStatus);
	}

	return Failure;
}
//---------------------------------------------------------------------

} //namespace AI