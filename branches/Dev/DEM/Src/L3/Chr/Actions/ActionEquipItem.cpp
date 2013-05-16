#include "ActionEquipItem.h"

#include <AI/Prop/PropActorBrain.h>
#include <Chr/Prop/PropEquipment.h>

namespace AI
{
__ImplementClass(AI::CActionEquipItem, 'AEQI', AI::CAction);

using namespace Properties;

bool CActionEquipItem::Activate(CActor* pActor)
{
	//!!!later play animation! //???where to attach model?
	CPropEquipment* pEquipment = pActor->GetEntity()->GetProperty<CPropEquipment>();
	return pEquipment ? pEquipment->Equip(Slot, pEquipment->FindItemStack(Item)) : false;
}
//---------------------------------------------------------------------

} //namespace AI