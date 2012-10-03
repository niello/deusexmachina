#pragma once
#ifndef __DEM_L3_ITEM_STACK_H__
#define __DEM_L3_ITEM_STACK_H__

#include "Item.h"

// Stack of items in the inventory. Describes some count of absolutely identical items & caches how much of
// them are equipped now, if owner is character. Count includes EquippedCount.

namespace Items
{

class CItemStack
{
private:

	PItem	Item;

	// DWORD is needed only for money in character's cash. Not inv item.
	// Items representing money will never exceed 65000 gold by the setting design
	WORD	Count;
	WORD	EquippedCount;

public:

	int		ID; // Database record ID

	CItemStack(): ID(0), Item(NULL), Count(0), EquippedCount(0) {}
	CItemStack(PItem pItem, WORD Num = 1): ID(0), Item(pItem), Count(Num), EquippedCount(0) { n_assert(pItem.isvalid() && Num > 0); }
	CItemStack(const CItemStack& pItem): ID(0), Item(pItem.Item), Count(pItem.Count), EquippedCount(pItem.EquippedCount) { n_assert(pItem.Item.isvalid()); }

	void			Add(WORD Num) { SetCount(Count + Num); }
	void			Remove(WORD Num) { n_assert(Num < Count); SetCount(Count - Num); }
	void			Equip(WORD Num) { SetEquippedCount(EquippedCount + Num); }
	void			Unequip(WORD Num) { n_assert(Num <= EquippedCount); SetEquippedCount(EquippedCount - Num); }
	void			Merge(const CItemStack* pOther);
	void			Clear() { Item = NULL; Count = EquippedCount = 0; }
	bool			IsValid() const { return Item.isvalid() && Count > 0; }

	void			SetItem(PItem NewItem) { n_assert(NewItem.isvalid()); Item = NewItem; }
	void			SetCount(WORD NewCount) { n_assert(NewCount > 0 && NewCount >= EquippedCount); Count = NewCount; }
	void			SetEquippedCount(WORD NewCount) { n_assert(NewCount <= Count); EquippedCount = NewCount; }
	CStrID			GetItemID() const { return Item->GetID(); }
	Ptr<CItemTpl>	GetTpl() const { return Item->GetTpl(); }
	CItem*			GetItem() const { return Item.get_unsafe(); }
	WORD			GetCount() const { return Count; }
	WORD			GetEquippedCount() const { return EquippedCount; }
	WORD			GetNotEquippedCount() const { return Count - EquippedCount; }
	float			GetWeight() const { return GetTpl()->Weight * Count; }
	float			GetVolume() const { return GetTpl()->Volume * (Count - EquippedCount); }
};
//---------------------------------------------------------------------

inline void CItemStack::Merge(const CItemStack* pOther)
{
	n_assert(pOther->GetItem()->IsEqual(Item));
	Add(pOther->GetCount());
	Equip(pOther->GetEquippedCount());
}
//---------------------------------------------------------------------

}

#endif