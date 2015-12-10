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

	//???pack? or allow UPTR?
	U16	Count;
	U16	EquippedCount;

public:

	CItemStack(): Item(NULL), Count(0), EquippedCount(0) {}
	CItemStack(PItem pItem, U16 Num = 1): Item(pItem), Count(Num), EquippedCount(0) { n_assert(pItem.IsValidPtr() && Num > 0); }
	CItemStack(const CItemStack& pItem): Item(pItem.Item), Count(pItem.Count), EquippedCount(pItem.EquippedCount) { n_assert(pItem.Item.IsValidPtr()); }

	void			Add(U16 Num) { SetCount(Count + Num); }
	void			Remove(U16 Num) { n_assert(Num <= Count); SetCount(Count - Num); }
	void			Equip(U16 Num) { SetEquippedCount(EquippedCount + Num); }
	void			Unequip(U16 Num) { n_assert(Num <= EquippedCount); SetEquippedCount(EquippedCount - Num); }
	void			Merge(const CItemStack* pOther);
	void			Clear() { Item = NULL; Count = EquippedCount = 0; }
	bool			IsValid() const { return Item.IsValidPtr() && Count > 0; }

	void			SetItem(PItem NewItem) { n_assert(NewItem.IsValidPtr()); Item = NewItem; }
	void			SetCount(U16 NewCount) { n_assert(NewCount >= EquippedCount); Count = NewCount; }
	void			SetEquippedCount(U16 NewCount) { n_assert(NewCount <= Count); EquippedCount = NewCount; }
	CStrID			GetItemID() const { return Item->GetID(); }
	Ptr<CItemTpl>	GetTpl() const { return Item->GetTpl(); }
	CItem*			GetItem() const { return Item.GetUnsafe(); }
	U16				GetCount() const { return Count; }
	U16				GetEquippedCount() const { return EquippedCount; }
	U16				GetNotEquippedCount() const { return Count - EquippedCount; }
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