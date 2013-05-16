#include "ActionTplEquipItem.h"

#include <AI/Prop/PropActorBrain.h>
#include <Chr/Prop/PropEquipment.h>
#include <Chr/Actions/ActionEquipItem.h>

#ifdef __WIN32__
	#ifdef GetProp
		#undef GetProp
		#undef SetProp
	#endif
#endif

namespace AI
{
__ImplementClass(AI::CActionTplEquipItem, 'ATEI', AI::CActionTpl);

using namespace Properties;

void CActionTplEquipItem::Init(PParams Params)
{
	CActionTpl::Init(Params);
	WSEffects.SetProp(WSP_ItemEquipped, WSP_ItemEquipped);
}
//---------------------------------------------------------------------

bool CActionTplEquipItem::GetPreconditions(CActor* pActor, CWorldState& WS, const CWorldState& WSGoal) const
{
	CPropInventory* pInv = pActor->GetEntity()->GetProperty<CPropInventory>();
	if (pInv->HasItem(WSGoal.GetProp(WSP_ItemEquipped))) FAIL;
	WS.SetProp(WSP_HasItem, WSP_ItemEquipped);
	OK;
}
//---------------------------------------------------------------------

bool CActionTplEquipItem::ValidateContextPreconditions(CActor* pActor, const CWorldState& WSGoal)
{
	CPropEquipment* pEquipment = pActor->GetEntity()->GetProperty<CPropEquipment>();
	//!!!check can equip!
	return !!pEquipment;
}
//---------------------------------------------------------------------

PAction CActionTplEquipItem::CreateInstance(const CWorldState& Context) const
{
	PActionEquipItem Act = n_new(CActionEquipItem);
	Act->Init(Context.GetProp(WSP_ItemEquipped));
	return Act.GetUnsafe();
}
//---------------------------------------------------------------------

} //namespace AI