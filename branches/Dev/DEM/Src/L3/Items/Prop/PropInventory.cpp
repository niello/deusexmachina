#include "PropInventory.h"

#include <Items/ItemManager.h>
#include <Game/EntityManager.h>
#include <Events/EventManager.h>
#include <Data/DataArray.h>

namespace Prop
{
__ImplementClass(Prop::CPropInventory, 'PINV', Game::CProperty);
__ImplementPropertyStorage(CPropInventory);

bool CPropInventory::InternalActivate()
{
	n_assert(CurrWeight == 0.f && CurrVolume == 0.f);

	Data::PDataArray InvDesc = GetEntity()->GetAttr<Data::PDataArray>(CStrID("Inventory"), NULL);
	if (InvDesc.IsValid() && InvDesc->GetCount())
	{
		CItemStack* pStack = Items.Reserve(InvDesc->GetCount());
		for (int i = 0; i < InvDesc->GetCount(); ++i, ++pStack)
		{
			Data::PParams StackDesc = InvDesc->Get<Data::PParams>(i);

			PItem Item = ItemMgr->GetItemTpl(StackDesc->Get<CStrID>(CStrID("ID")))->GetTemplateItem();
			Data::PParams ItemInst = StackDesc->Get<Data::PParams>(CStrID("Instance"), NULL);
			if (ItemInst.IsValid())
			{
				Item = Item->Clone();
				n_error("IMPLEMENT ME!!!");
				//!!!load per-instance fields!
			}
			pStack->SetItem(Item);
			pStack->SetCount((WORD)StackDesc->Get<int>(CStrID("Count")));
			//???load EquippedCount?

			CurrWeight += pStack->GetWeight();
			CurrVolume += pStack->GetVolume();
		}

		n_assert(MaxWeight < 0.f || CurrWeight <= MaxWeight);
		n_assert(MaxVolume < 0.f || CurrVolume <= MaxVolume);
	}

	PROP_SUBSCRIBE_PEVENT(ExposeSI, CPropInventory, OnExposeSI);
	PROP_SUBSCRIBE_PEVENT(OnLevelSaving, CPropInventory, OnLevelSaving);
	OK;
}
//---------------------------------------------------------------------

void CPropInventory::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(ExposeSI);
	UNSUBSCRIBE_EVENT(OnLevelSaving);
}
//---------------------------------------------------------------------

bool CPropInventory::OnLevelSaving(const Events::CEventBase& Event)
{
	if (!Items.GetCount())
	{
		GetEntity()->DeleteAttr(CStrID("Inventory"));
		OK;
	}

	// Need to recreate array because else we may rewrite initial level desc in the HRD cache
	Data::PDataArray InvDesc = n_new(Data::CDataArray);
	GetEntity()->SetAttr<Data::PDataArray>(CStrID("Inventory"), InvDesc);

	CData* pData = InvDesc->Reserve(Items.GetCount());
	foreach_stack(Stack, Items)
	{
		n_assert_dbg(Stack->IsValid());

		Data::PParams StackDesc = n_new(Data::CParams(4));

		StackDesc->Set(CStrID("ID"), Stack->GetItemID());
		if (!Stack->GetItem()->IsTemplateInstance())
		{
			n_error("IMPLEMENT ME!!!");
			//!!!save per-instance fields!
		}
		StackDesc->Set(CStrID("Count"), (int)Stack->GetCount());
		//???save EquippedCount?

		*pData = StackDesc;
		++pData;
	}

	OK;
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
	n_printf("CEntity \"%s\": Item \"%s\" is too big or heavy\n", GetEntity()->GetUID(), NewItem->GetID());
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

}