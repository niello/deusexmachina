#include "PropEquipment.h"

#include <Items/ItemStack.h>
#include <Items/Prop/PropInventory.h>
#include <AI/Prop/PropActorBrain.h>
#include <AI/AIServer.h>
#include <Events/EventManager.h>

namespace Properties
{
__ImplementClass(Properties::CPropEquipment, 'PEQI', CPropInventory);

CPropEquipment::CPropEquipment()
{
	//!!!tmp!
	CSlot Slot;
	//???bitmask of acceptable types? will be much more efficient! types as bit enum, not as strids!
	Slot.Type = CStrID("Weapon");
	Slots.Add(CStrID("MainWpn"), Slot);
}
//---------------------------------------------------------------------

//!!!OnPropsActivated can setup equipment info loaded from DB! (need to load inventory contents before)
// at least it can setup equipment effects

//void CPropEquipment::Activate()
//{
//	Game::CProperty::Activate();
//}
////---------------------------------------------------------------------
//
//void CPropEquipment::Deactivate()
//{
//	Game::CProperty::Deactivate();
//}
////---------------------------------------------------------------------

void CPropEquipment::Save()
{
	CPropInventory::Save();
	
	DB::CDataset* DS = ItemMgr->GetEquipmentDataset();
	n_assert(DS);

	for (int i = 0; i < Slots.GetCount(); i++)
	{
		CSlot& Slot = Slots.ValueAtIndex(i);
		if (Slot.pStack)
		{
			DS->AddRow();
			DS->Set<int>(0, Slot.pStack->ID);
			DS->Set<CStrID>(1, Slots.KeyAtIndex(i));
			DS->Set<int>(2, Slot.Count);
		}
	}
}
//---------------------------------------------------------------------

void CPropEquipment::Load()
{
	CPropInventory::Load();

	DB::CDataset* DS = ItemMgr->GetEquipmentDataset();
	if (!DS) return;

	DB::PValueTable VT = DS->GetValueTable();

	if (!VT.IsValid() || !VT->GetRowCount()) return;

	foreach_stack(Stack, Items)
	{
		int Start = 0;
		int End = VT->GetRowCount() - 1;
		while (Start <= End)
		{
			int CurrIdx = Start + ((End - Start) >> 1);
			int VTID = VT->Get<int>(0, CurrIdx);
			if (VTID == Stack->ID)
			{
				Equip(VT->Get<CStrID>(1, CurrIdx), Stack, VT->Get<int>(2, CurrIdx));
				break;
			}
			else if (VTID < Stack->ID) Start = CurrIdx + 1;
			else End = CurrIdx - 1;
		}
	}
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
		if (Idx != INVALID_INDEX && Slots.ValueAtIndex(Idx).Type == pStack->GetTpl()->GetType())
			pSlot = &Slots.ValueAtIndex(Idx);
	}
	else
	{
		// Find any appropriate slot
		for (int i = 0; i < Slots.GetCount(); ++i)
			if (Slots.ValueAtIndex(i).Type == pStack->GetTpl()->GetType())
			{
				pSlot = &Slots.ValueAtIndex(i);
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

} // namespace Properties