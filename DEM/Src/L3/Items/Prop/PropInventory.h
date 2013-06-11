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

	nArray<CItemStack> Items; //???linked list?

	typedef nArray<CItemStack>::iterator ItItemStack;

	virtual bool	InternalActivate();
	virtual void	InternalDeactivate();
	WORD			RemoveItem(ItItemStack Stack, WORD Count, bool AsManyAsCan);

	DECLARE_EVENT_HANDLER(ExposeSI, OnExposeSI);
	DECLARE_EVENT_HANDLER(OnSave, OnSave);
	DECLARE_EVENT_HANDLER(OnLoad, OnLoad);
	DECLARE_EVENT_HANDLER(OnLoadAfter, OnLoadAfter);

	virtual void ExposeSI();
	virtual void Save();
	virtual void Load();

public:

	float	MaxWeight; // < 0 = not limited
	float	MaxVolume; // < 0 = not limited
	float	CurrWeight;
	float	CurrVolume;

	//???MaxWV to attrs?
	CPropInventory(): MaxWeight(82.f), MaxVolume(146.f), CurrWeight(0.f), CurrVolume(0.f) {}
	//virtual ~CPropInventory();

	bool			AddItem(PItem NewItem, WORD Count = 1);
	bool			AddItem(CStrID ItemID, WORD Count = 1);
	bool			AddItem(const CItemStack& Items);
	WORD			RemoveItem(PItem Item, WORD Count = 1, bool AsManyAsCan = false);
	WORD			RemoveItem(CStrID ItemID, WORD Count = 1, bool AsManyAsCan = false);
	bool			HasItem(CStrID ItemID, WORD Count = 1); //???GetItemCount(CStrID)?
	CItemStack*		FindItemStack(PItem Item);
	CItemStack*		FindItemStack(CStrID ItemID);
	bool			SplitItems(PItem Item, WORD Ñount, CItemStack& OutStack);
	void			MergeItems(PItem Item);

	const nArray<CItemStack>& GetItems() const { return Items; }
};
//---------------------------------------------------------------------

inline bool CPropInventory::AddItem(CStrID ItemID, WORD Count)
{
	return AddItem(ItemMgr->GetItemTpl(ItemID)->GetTemplateItem(), Count);
}
//---------------------------------------------------------------------

inline bool CPropInventory::AddItem(const CItemStack& Items)
{
	return AddItem(Items.GetItem(), Items.GetCount());
}
//---------------------------------------------------------------------

inline CItemStack* CPropInventory::FindItemStack(PItem Item)
{
	foreach_stack(Stack, Items)
		if (Stack->GetItem()->IsEqual(Item)) return (CItemStack*)Stack;
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