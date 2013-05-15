#include "ActionTplPickItemWorld.h"

#include <AI/Behaviour/Action.h>
#include <AI/Prop/PropActorBrain.h>
#include <AI/Perception/Stimulus.h>
#include <AI/Memory/MemFactSmartObj.h>
#include <Game/EntityManager.h>
#include <Items/Prop/PropItem.h>

#ifdef __WIN32__
	#ifdef GetProp
		#undef GetProp
		#undef SetProp
	#endif
#endif

namespace AI
{
__ImplementClassNoFactory(AI::CActionTplPickItemWorld, AI::CActionTpl);
__ImplementClass(AI::CActionTplPickItemWorld);

using namespace Properties;

void CActionTplPickItemWorld::Init(PParams Params)
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

	CMemFactNode* pCurr = pActor->GetMemSystem().GetFactsByType(CMemFactSmartObj::RTTI);
	for (; pCurr; pCurr = pCurr->GetSucc())
	{
		CMemFactSmartObj* pSOFact = (CMemFactSmartObj*)pCurr->Object.CStr();
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

} //namespace AI