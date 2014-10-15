#include "ActionTplPickItemWorld.h"

#include <AI/Behaviour/Action.h>
#include <AI/PropActorBrain.h>
#include <AI/Perception/Stimulus.h>
#include <AI/Memory/MemFactSmartObj.h>
#include <Game/EntityManager.h>
#include <Items/Prop/PropItem.h>

namespace AI
{
__ImplementClass(AI::CActionTplPickItemWorld, 'ATIW', AI::CActionTpl);

using namespace Prop;

void CActionTplPickItemWorld::Init(Data::PParams Params)
{
	CActionTpl::Init(Params);
	WSEffects.SetProp(WSP_HasItem, WSP_HasItem);
}
//---------------------------------------------------------------------

bool CActionTplPickItemWorld::GetPreconditions(CActor* pActor, CWorldState& WS, const CWorldState& WSGoal) const
{
	WS.SetProp(WSP_UsingSmartObj, ItemEntityID);
	WS.SetProp(WSP_Action, CStrID("PickItem"));
	OK;
}
//---------------------------------------------------------------------

bool CActionTplPickItemWorld::ValidateContextPreconditions(CActor* pActor, const CWorldState& WSGoal)
{
	CMemFactSmartObj* pBest = NULL;
	float MaxConf = 0.f;

	CMemFactNode It = pActor->GetMemSystem().GetFactsByType(CMemFactSmartObj::RTTI);
	for (; It; ++It)
	{
		CMemFactSmartObj* pSOFact = (CMemFactSmartObj*)It->Get();
		if (pSOFact->TypeID == CStrID("Item"))
		{
			Game::CEntity* pEnt = EntityMgr->GetEntity(pSOFact->pSourceStimulus->SourceEntityID);
			CPropItem* pItemProp = pEnt ? pEnt->GetProperty<CPropItem>() : NULL;
			if (pItemProp &&
				pSOFact->Confidence > MaxConf &&
				pItemProp->Items.GetItemID() == (CStrID)WSGoal.GetProp(WSP_HasItem))
			{
				pBest = pSOFact;
				MaxConf = pSOFact->Confidence;
			}
		}
	}

	if (!pBest) FAIL;

	ItemEntityID = pBest->pSourceStimulus->SourceEntityID;

	OK;
}
//---------------------------------------------------------------------

PAction CActionTplPickItemWorld::CreateInstance(const CWorldState& Context) const
{
	return NULL;
}
//---------------------------------------------------------------------

}