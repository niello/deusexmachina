#include "ActionTplEquipItem.h"

#include <AI/PropActorBrain.h>
#include <Items/Prop/PropEquipment.h>
#include <Items/Actions/ActionEquipItem.h>
#include <Core/Factory.h>

namespace AI
{
FACTORY_CLASS_IMPL(AI::CActionTplEquipItem, 'ATEI', AI::CActionTpl);

using namespace Prop;

void CActionTplEquipItem::Init(Data::PParams Params)
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
	return Act.Get();
}
//---------------------------------------------------------------------

} //namespace AI