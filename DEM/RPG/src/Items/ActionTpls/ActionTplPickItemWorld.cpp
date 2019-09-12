#include "ActionTplPickItemWorld.h"

#include <AI/Behaviour/Action.h>
#include <AI/PropActorBrain.h>
#include <AI/Perception/Stimulus.h>
#include <AI/Memory/MemFactSmartObj.h>
#include <Game/GameServer.h>
#include <Items/Prop/PropItem.h>
#include <Core/Factory.h>

namespace AI
{
__ImplementClass(AI::CActionTplPickItemWorld, 'ATIW', AI::CActionTpl);

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
	CMemFactSmartObj* pBest = nullptr;
	float MaxConf = 0.f;

	CStrID DesiredItemID = (CStrID)WSGoal.GetProp(WSP_HasItem);

	CMemFactNode It = pActor->GetMemSystem().GetFactsByType(CMemFactSmartObj::RTTI);
	for (; It; ++It)
	{
		CMemFactSmartObj* pSOFact = (CMemFactSmartObj*)It->Get();
		if (pSOFact->TypeID == CStrID("Item"))
		{
			Prop::CPropItem* pItemProp = GameSrv->GetEntityMgr()->GetProperty<Prop::CPropItem>(pSOFact->pSourceStimulus->SourceEntityID);
			if (pItemProp &&
				pSOFact->Confidence > MaxConf &&
				pItemProp->Items.GetItemID() == DesiredItemID)
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
	return nullptr;
}
//---------------------------------------------------------------------

}