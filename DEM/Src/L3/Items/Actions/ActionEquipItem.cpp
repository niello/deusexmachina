#include "ActionEquipItem.h"

#include <AI/PropActorBrain.h>
#include <Items/Prop/PropEquipment.h>
#include <Core/Factory.h>

namespace AI
{
__ImplementClass(AI::CActionEquipItem, 'AEQI', AI::CAction);

bool CActionEquipItem::Activate(CActor* pActor)
{
	//!!!later play animation! //???where to attach model?
	Prop::CPropEquipment* pEquipment = pActor->GetEntity()->GetProperty<Prop::CPropEquipment>();
	return pEquipment ? pEquipment->Equip(Slot, pEquipment->FindItemStack(Item)) : false;
}
//---------------------------------------------------------------------

}