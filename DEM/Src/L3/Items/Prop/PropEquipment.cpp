#include "PropEquipment.h"

#include <Items/ItemStack.h>
#include <Items/Prop/PropInventory.h>
#include <AI/PropActorBrain.h>
#include <AI/AIServer.h>
#include <Events/EventServer.h>

namespace Prop
{
__ImplementClass(Prop::CPropEquipment, 'PEQI', CPropInventory);

CPropEquipment::CPropEquipment()
{
	//!!!TMP!
	CSlot Slot;
	//???bitmask of acceptable types? will be much more efficient! types as bit enum, not as strids!
	Slot.Type = CStrID("Weapon");
	Slots.Add(CStrID("MainWpn"), Slot);
}
//---------------------------------------------------------------------

bool CPropEquipment::Equip(CStrID Slot, CItemStack* pStack, WORD Count)
{
	if (!pStack || !pStack->IsValid()) FAIL;

	CSlot* pSlot = NULL;

	//!!!Item type is in AllowedSlotTypes instead of Slot.Type check!

	if (Slot.IsValid())
	{
		int Idx = Slots.FindIndex(Slot);
		if (Idx != INVALID_INDEX && Slots.ValueAt(Idx).Type == pStack->GetTpl()->GetType())
			pSlot = &Slots.ValueAt(Idx);
	}
	else
	{
		// Find any appropriate slot
		for (int i = 0; i < Slots.GetCount(); ++i)
			if (Slots.ValueAt(i).Type == pStack->GetTpl()->GetType())
			{
				pSlot = &Slots.ValueAt(i);
				break;
			}
	}

	if (!pSlot) FAIL;

	//???need some action, animation etc?
	//!!!can support unequippable items, so Unequip will fail and this func will fail too!
	if (pSlot->pStack) Unequip(Slot);
	pSlot->pStack = pStack;
	pSlot->Count = Count;
	pStack->Equip(Count);

	//!!!apply effects!

	//!!!Fire OnItemEquipped!

	OK;
}
//---------------------------------------------------------------------

void CPropEquipment::Unequip(CStrID SlotID)
{
	CSlot& Slot = Slots[SlotID];

	if (Slot.pStack)
	{
		n_assert(Slot.Count > 0);

		//!!!remove effects!

		Slot.pStack->Unequip(Slot.Count);
		Slot.pStack = NULL;
		Slot.Count = 0;

		//!!!Fire OnItemUnequipped!
	}
}
//---------------------------------------------------------------------

}