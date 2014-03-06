#pragma once
#ifndef __DEM_L3_PROP_INVENTORY_H__
#define __DEM_L3_PROP_INVENTORY_H__

#include <Game/Property.h>
#include <Items/ItemManager.h>
#include <Items/ItemStack.h>

// Item collection (character inventory or container items list). When character dies this prop
// does not change and is used by dead body container. Equipped items are added to it.

// Adds Actor actions available:
// - Pick

//!!!adds IAO action OpenContainer/PickPocket/SeeInventory!

#define foreach_stack(Stack, Collection) \
	ItItemStack Stack = Collection.Begin(); \
	for (; Stack != Collection.End(); ++Stack)

namespace Prop
{
using namespace Items;

class CPropInventory: public Game::CProperty
{
	__DeclareClass(CPropInventory);
	__DeclarePropertyStorage;

protected:

	CArray<CItemStack> Items; //???linked list?

	typedef CArray<CItemStack>::CIterator ItItemStack;

	virtual bool	InternalActivate();
	virtual void	InternalDeactivate();
	virtual void	EnableSI(class CPropScriptable& Prop);
	virtual void	DisableSI(class CPropScriptable& Prop);

	WORD			RemoveItem(ItItemStack Stack, WORD Count, bool AsManyAsCan);

	DECLARE_EVENT_HANDLER(OnPropActivated, OnPropActivated);
	DECLARE_EVENT_HANDLER(OnPropDeactivating, OnPropDeactivating);
	DECLARE_EVENT_HANDLER(OnLevelSaving, OnLevelSaving);
	DECLARE_EVENT_HANDLER(OnSOActionDone, OnSOActionDone);

public:

	float	MaxWeight; // < 0 = not limited
	float	MaxVolume; // < 0 = not limited
	float	CurrWeight;
	float	CurrVolume;

	//???MaxWV to attrs?
	CPropInventory(): MaxWeight(82.f), MaxVolume(146.f), CurrWeight(0.f), CurrVolume(0.f) {}
	//virtual ~CPropInventory();

	WORD			AddItem(PItem NewItem, WORD Count = 1, bool AsManyAsCan = false);
	WORD			AddItem(CStrID ItemID, WORD Count = 1, bool AsManyAsCan = false);
	WORD			AddItem(const CItemStack& Items, bool AsManyAsCan = false);
	WORD			RemoveItem(PItem Item, WORD Count = 1, bool AsManyAsCan = false);
	WORD			RemoveItem(CStrID ItemID, WORD Count = 1, bool AsManyAsCan = false);
	bool			HasItem(CStrID ItemID, WORD Count = 1); //???GetItemCount(CStrID)?
	CItemStack*		FindItemStack(const CItem* pItem);
	CItemStack*		FindItemStack(CStrID ItemID);
	bool			SplitItems(PItem Item, WORD Ñount, CItemStack& OutStack);
	void			MergeItems(PItem Item);

	const CArray<CItemStack>& GetItems() const { return Items; }
};
//---------------------------------------------------------------------

inline WORD CPropInventory::AddItem(CStrID ItemID, WORD Count, bool AsManyAsCan)
{
	return AddItem(ItemMgr->GetItemTpl(ItemID)->GetTemplateItem(), Count, AsManyAsCan);
}
//---------------------------------------------------------------------

inline WORD CPropInventory::AddItem(const CItemStack& Items, bool AsManyAsCan)
{
	return AddItem(Items.GetItem(), Items.GetCount(), AsManyAsCan);
}
//---------------------------------------------------------------------

inline CItemStack* CPropInventory::FindItemStack(const CItem* pItem)
{
	if (!pItem) return NULL;
	foreach_stack(Stack, Items)
		if (Stack->GetItem()->IsEqual(pItem)) return (CItemStack*)Stack;
	return NULL;
}
//---------------------------------------------------------------------

inline CItemStack* CPropInventory::FindItemStack(CStrID ItemID)
{
	foreach_stack(Stack, Items)
		if (Stack->GetItem()->GetID() == ItemID) return (CItemStack*)Stack;
	return NULL;
}
//---------------------------------------------------------------------

}

#endif