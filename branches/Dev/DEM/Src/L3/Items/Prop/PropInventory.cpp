#include "PropInventory.h"

#include <Items/ItemManager.h>
#include <Events/EventManager.h>
#include <Game/EntityManager.h>

const nString StrInventories("Inventories");

namespace Prop
{
__ImplementClass(Prop::CPropInventory, 'PINV', Game::CProperty);
__ImplementPropertyStorage(CPropInventory);

void CPropInventory::Activate()
{
	Game::CProperty::Activate();

	n_assert(CurrWeight == 0.f && CurrVolume == 0.f);

	PROP_SUBSCRIBE_PEVENT(ExposeSI, CPropInventory, OnExposeSI);
	PROP_SUBSCRIBE_PEVENT(OnSave, CPropInventory, OnSave);
	PROP_SUBSCRIBE_PEVENT(OnLoad, CPropInventory, OnLoad);
	PROP_SUBSCRIBE_PEVENT(OnLoadAfter, CPropInventory, OnLoadAfter);
}
//---------------------------------------------------------------------

void CPropInventory::Deactivate()
{
	UNSUBSCRIBE_EVENT(ExposeSI);
	UNSUBSCRIBE_EVENT(OnSave);
	UNSUBSCRIBE_EVENT(OnLoad);
	UNSUBSCRIBE_EVENT(OnLoadAfter);
	Game::CProperty::Deactivate();
}
//---------------------------------------------------------------------

bool CPropInventory::OnSave(const Events::CEventBase& Event)
{
	Save();
	OK;
}
//---------------------------------------------------------------------

bool CPropInventory::OnLoad(const Events::CEventBase& Event)
{
	Load();
	OK;
}
//---------------------------------------------------------------------

bool CPropInventory::OnLoadAfter(const Events::CEventBase& Event)
{
	CurrWeight = 0.f;
	CurrVolume = 0.f;
	foreach_stack(Stack, Items)
	{
		CurrWeight += Stack->GetWeight();
		CurrVolume += Stack->GetVolume();
	}
	n_assert(MaxWeight < 0.f || CurrWeight <= MaxWeight);
	n_assert(MaxVolume < 0.f || CurrVolume <= MaxVolume);
	OK;
}
//---------------------------------------------------------------------

void CPropInventory::Save()
{
/*
	CDataset* DS = ItemMgr->GetInventoriesDataset();
	if (!DS) return;

	PValueTable VT = DS->GetValueTable();
	int RowCount = ItemMgr->GetInventoriesRowCount();

	CStrID EntID = GetEntity()->GetUID();

	// Cause VT column order never changes we use indices directly, without lookup
	// VT is sorted by ItemOwner. We can't guarantee asc/desc order of CStrIDs, so we use linear search,
	// but we can guarantee grouping of rows by ItemOwner, so we search for our block start.
	// IsRowUntouched check is used as optimization, since all modified rows aren't ours.
	int FirstRowIdx;
	for (FirstRowIdx = 0; FirstRowIdx < RowCount; ++FirstRowIdx)
		if (VT->IsRowUntouched(FirstRowIdx) && VT->Get<CStrID>(1, FirstRowIdx) == EntID)
			break;

	int StopRowIdx;
	if (FirstRowIdx < RowCount)
	{
		StopRowIdx = FirstRowIdx + 1;
		while (	StopRowIdx < RowCount &&
				VT->IsRowUntouched(StopRowIdx) &&
				VT->Get<CStrID>(1, StopRowIdx) == EntID)
			++StopRowIdx;
	}
	else StopRowIdx = FirstRowIdx;

	foreach_stack(Stack, Items)
	{
		n_assert(Stack->IsValid());

		int RowIdx;
		bool Found = false;

		// New stacks have zero ID by design, so we always know
		// whether to search or to create new row
		//!!!Ordering by (ItemOwner, ID) may be faster cause we can binary-search slot row
		if (Stack->ID > 0)
		{
			for (RowIdx = FirstRowIdx; RowIdx < StopRowIdx; ++RowIdx)
				if (VT->Get<int>(0, RowIdx) == Stack->ID)
				{
					Found = true;
					break;
				}
		}
		else Stack->ID = ItemMgr->NewItemStackID();

		if (!Found)
		{
			RowIdx = VT->AddRow();
			VT->Set<int>(0, RowIdx, Stack->ID);
			VT->Set<CStrID>(1, RowIdx, EntID);
		}

		VT->Set<CStrID>(2, RowIdx, Stack->GetItemID()); //???can change or is R/O?
		if (Stack->GetItem()->IsTemplateInstance())
			VT->Set<int>(3, RowIdx, -1);
		else
		{
			n_assert(false);
			//???can instance use ID of stack? instances count > stacks count can never happen
			// so int InstID will be bool IsTplInstance, if false, get from inst table by ID
			// Item entitiy should then save ItemID field & get ID for its Stack
			//!!!save instance, get its ID & save ID!
		}
		VT->Set<int>(4, RowIdx, (int)Stack->GetCount());
	}

	for (int RowIdx = FirstRowIdx; RowIdx < StopRowIdx; ++RowIdx)
		if (VT->IsRowUntouched(RowIdx))
			VT->DeleteRow(RowIdx);
*/
}
//---------------------------------------------------------------------

void CPropInventory::Load()
{
/*
	//Items.Clear();
	n_assert(Items.GetCount() == 0);

	CDataset* DS = ItemMgr->GetInventoriesDataset();
	if (!DS) return;

	PValueTable VT = DS->GetValueTable();

	if (!VT.IsValid() || !VT->GetRowCount()) return;

	CStrID EntID = GetEntity()->GetUID();

	int RowIdx;
	for (RowIdx = 0; RowIdx < VT->GetRowCount(); ++RowIdx)
		if (VT->Get<CStrID>(1, RowIdx) == EntID)
			break;

	while (RowIdx < VT->GetRowCount() && VT->Get<CStrID>(1, RowIdx) == EntID)
	{
		PItem Item;
		int InstID = VT->Get<int>(3, RowIdx);
		if (InstID > -1)
		{
			n_assert(false);
			//int InstID = VT->Get<int>(3, RowIdx);
			//Item = ItemMgr->LoadItemInstance(InstID);
		}
		else Item = ItemMgr->GetItemTpl(VT->Get<CStrID>(2, RowIdx))->GetTemplateItem();

		CItemStack New(Item, (WORD)VT->Get<int>(4, RowIdx));
		New.ID = VT->Get<int>(0, RowIdx);
		Items.Append(New);

		++RowIdx;
	}
*/
}
//---------------------------------------------------------------------

bool CPropInventory::AddItem(PItem NewItem, WORD Count)
{
	float TotalW = NewItem->GetTpl()->Weight * Count;
	float TotalV = NewItem->GetTpl()->Volume * Count;

	if ((MaxWeight < 0.f || TotalW <= (MaxWeight - CurrWeight)) &&
		(MaxVolume < 0.f || TotalV <= (MaxVolume - CurrVolume)))
	{
		CurrWeight += TotalW;
		CurrVolume += TotalV;

		foreach_stack(Stack, Items)
			if (Stack->GetItem()->IsEqual(NewItem))
			{
				Stack->Add(Count);
				break;
			}

		if (Stack == Items.End()) Items.Append(CItemStack(NewItem, Count)); //???!!!preallocate & set fields?!

		PParams P = n_new(CParams);
		P->Set(CStrID("Item"), nString(NewItem->GetID().CStr()));
		P->Set(CStrID("Count"), (int)Count);
		P->Set(CStrID("Entity"), nString(GetEntity()->GetUID().CStr()));
		EventMgr->FireEvent(CStrID("OnItemAdded"), P);

		OK;
	}

#ifdef _DEBUG
	n_printf("CEntity \"%s\": Item \"%s\" is too big or heavy\n",
		GetEntity()->GetUID(),
		NewItem->GetID());
#endif

	FAIL;
}
//---------------------------------------------------------------------

WORD CPropInventory::RemoveItem(ItItemStack Stack, WORD Count, bool AsManyAsCan)
{
	if (Stack == Items.End()) FAIL;
	
	//???some flag AllowRemoveEquipped?

	WORD ToRemove;
	if (Stack->GetCount() >= Count) ToRemove = Count;
	else if (AsManyAsCan) ToRemove = Stack->GetCount();
	else return 0;

	CurrWeight -= Stack->GetTpl()->Weight * ToRemove;
	CurrVolume -= Stack->GetVolume();
	
	PParams P = n_new(CParams);
	P->Set(CStrID("Item"), nString(Stack->GetItemID().CStr()));
	P->Set(CStrID("Count"), (int)ToRemove);
	P->Set(CStrID("Entity"), nString(GetEntity()->GetUID().CStr()));

	if (Stack->GetCount() > ToRemove)
	{
		Stack->Remove(ToRemove);
		CurrVolume += Stack->GetVolume();
	}
	else Items.Erase(Stack);

	EventMgr->FireEvent(CStrID("OnItemRemoved"), P);

	return ToRemove;
}
//---------------------------------------------------------------------

WORD CPropInventory::RemoveItem(PItem Item, WORD Count, bool AsManyAsCan)
{
	foreach_stack(Stack, Items)
		if (Stack->GetItem()->IsEqual(Item)) break;
	return RemoveItem(Stack, Count, AsManyAsCan) > 0;
}
//---------------------------------------------------------------------

WORD CPropInventory::RemoveItem(CStrID ItemID, WORD Count, bool AsManyAsCan)
{
	WORD LeftToRemove = Count;
	foreach_stack(Stack, Items)
		if (Stack->GetItemID() == ItemID)
		{
			LeftToRemove -= RemoveItem(Stack, LeftToRemove, AsManyAsCan);
			if (!LeftToRemove) return Count;
		}
	return Count - LeftToRemove;
}
//---------------------------------------------------------------------

bool CPropInventory::HasItem(CStrID ItemID, WORD Count)
{
	foreach_stack(Stack, Items)
		if (Stack->GetItemID() == ItemID)
		{
			if (Stack->GetCount() < Count) Count -= Stack->GetCount();
			else OK;
		}
	FAIL;
}
//---------------------------------------------------------------------

bool CPropInventory::SplitItems(PItem Item, WORD Count, CItemStack& OutStack)
{
	foreach_stack(Stack, Items)
		if (Stack->GetItem()->IsEqual(Item)) break;

	if (Stack == Items.End()) FAIL;

	if (Stack->GetCount() == Count)
	{
		OutStack = *Stack;
		OK;
	}
	else if (Stack->GetCount() < Count) FAIL;

	Stack->Remove(Count);

	OutStack.SetItem(Item);
	//OutStack.SetEquippedCount(0); //???how to behave when split equipped?
	OutStack.SetCount(Count);

	Items.Append(OutStack); //???always append? what about stack-on-cursor? append too?
	OK;
}
//---------------------------------------------------------------------

// Merge all items into one stack
void CPropInventory::MergeItems(PItem Item)
{
	foreach_stack(MainStack, Items)
		if (MainStack->GetItem()->IsEqual(Item)) break;

	if (MainStack == Items.End()) return;

	for (int i = Items.GetCount() - 1; i >= 0; i--)
		if (Items[i].GetItem()->IsEqual(Item))
		{
			MainStack->Add(Items[i].GetCount());
			//???what about equipped? MainStack->Merge(Items[i]);?
			//in normal case erasure will be executed only once
			Items.EraseAt(i);
		}
}
//---------------------------------------------------------------------

} // namespace Prop