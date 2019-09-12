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

class CPropInventory: public Game::CProperty
{
	__DeclareClass(CPropInventory);
	__DeclarePropertyStorage;

protected:

	CArray<Items::CItemStack> Items; //???linked list?

	typedef CArray<Items::CItemStack>::CIterator ItItemStack;

	virtual bool	InternalActivate();
	virtual void	InternalDeactivate();
	virtual void	EnableSI(class CPropScriptable& Prop);
	virtual void	DisableSI(class CPropScriptable& Prop);

	U16				RemoveItem(ItItemStack Stack, U16 Count, bool AsManyAsCan);

	DECLARE_EVENT_HANDLER(OnPropActivated, OnPropActivated);
	DECLARE_EVENT_HANDLER(OnPropDeactivating, OnPropDeactivating);
	DECLARE_EVENT_HANDLER(OnLevelSaving, OnLevelSaving);
	DECLARE_EVENT_HANDLER(OnSOActionDone, OnSOActionDone);

public:

	float	MaxWeight = -1.f; // < 0 = not limited
	float	MaxVolume = -1.f; // < 0 = not limited
	float	CurrWeight = 0.f;
	float	CurrVolume = 0.f;

	//???MaxWV to attrs?
	virtual ~CPropInventory();

	U16					AddItem(Items::PItem NewItem, U16 Count = 1, bool AsManyAsCan = false);
	U16					AddItem(CStrID ItemID, U16 Count = 1, bool AsManyAsCan = false);
	U16					AddItem(const Items::CItemStack& Items, bool AsManyAsCan = false);
	U16					RemoveItem(Items::PItem Item, U16 Count = 1, bool AsManyAsCan = false);
	U16					RemoveItem(CStrID ItemID, U16 Count = 1, bool AsManyAsCan = false);
	bool				HasItem(CStrID ItemID, U16 Count = 1); //???GetItemCount(CStrID)?
	Items::CItemStack*	FindItemStack(const Items::CItem* pItem);
	Items::CItemStack*	FindItemStack(CStrID ItemID);
	bool				SplitItems(Items::PItem Item, U16 Ñount, Items::CItemStack& OutStack);
	void				MergeItems(Items::PItem Item);

	const CArray<Items::CItemStack>& GetItems() const { return Items; }
};
//---------------------------------------------------------------------

inline U16 CPropInventory::AddItem(CStrID ItemID, U16 Count, bool AsManyAsCan)
{
	Items::PItemTpl Tpl = ItemMgr->GetItemTpl(ItemID);
	return Tpl.IsValidPtr() ? AddItem(Tpl->GetTemplateItem(), Count, AsManyAsCan) : 0;
}
//---------------------------------------------------------------------

inline U16 CPropInventory::AddItem(const Items::CItemStack& Items, bool AsManyAsCan)
{
	return AddItem(Items.GetItem(), Items.GetCount(), AsManyAsCan);
}
//---------------------------------------------------------------------

inline Items::CItemStack* CPropInventory::FindItemStack(const Items::CItem* pItem)
{
	if (!pItem) return nullptr;
	foreach_stack(Stack, Items)
		if (Stack->GetItem()->IsEqual(pItem)) return (Items::CItemStack*)Stack;
	return nullptr;
}
//---------------------------------------------------------------------

inline Items::CItemStack* CPropInventory::FindItemStack(CStrID ItemID)
{
	foreach_stack(Stack, Items)
		if (Stack->GetItem()->GetID() == ItemID) return (Items::CItemStack*)Stack;
	return nullptr;
}
//---------------------------------------------------------------------

}

#endif