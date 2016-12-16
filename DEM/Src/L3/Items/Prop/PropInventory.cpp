#include "PropInventory.h"

#include <Items/ItemManager.h>
#include <Items/Prop/PropItem.h>
#include <Scripting/PropScriptable.h>
#include <Game/GameServer.h>
#include <Game/Entity.h>
#include <Events/EventServer.h>
#include <Data/DataArray.h>
#include <Math/Math.h>

namespace Prop
{
__ImplementClass(Prop::CPropInventory, 'PINV', Game::CProperty);
__ImplementPropertyStorage(CPropInventory);

bool CPropInventory::InternalActivate()
{
	n_assert(CurrWeight == 0.f && CurrVolume == 0.f);

	Data::PDataArray InvDesc;
	if (GetEntity()->GetAttr<Data::PDataArray>(InvDesc, CStrID("Inventory")) && InvDesc->GetCount())
	{
		Items::CItemStack* pStack = Items.Reserve(InvDesc->GetCount());
		for (UPTR i = 0; i < InvDesc->GetCount(); ++i, ++pStack)
		{
			Data::PParams StackDesc = InvDesc->Get<Data::PParams>(i);

			Items::PItem Item = ItemMgr->GetItemTpl(StackDesc->Get<CStrID>(CStrID("ID")))->GetTemplateItem();
			Data::PParams ItemInst = StackDesc->Get<Data::PParams>(CStrID("Instance"), NULL);
			if (ItemInst.IsValidPtr())
			{
				Item = Item->Clone();
				Sys::Error("IMPLEMENT ME!!!");
				//!!!load per-instance fields!
			}
			pStack->SetItem(Item);
			pStack->SetCount((U16)StackDesc->Get<int>(CStrID("Count")));
			//???load EquippedCount?

			CurrWeight += pStack->GetWeight();
			CurrVolume += pStack->GetVolume();
		}

		n_assert(MaxWeight < 0.f || CurrWeight <= MaxWeight);
		n_assert(MaxVolume < 0.f || CurrVolume <= MaxVolume);
	}

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive()) EnableSI(*pProp);

	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropInventory, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnPropDeactivating, CPropInventory, OnPropDeactivating);
	PROP_SUBSCRIBE_PEVENT(OnLevelSaving, CPropInventory, OnLevelSaving);
	PROP_SUBSCRIBE_PEVENT(OnSOActionDone, CPropInventory, OnSOActionDone);
	OK;
}
//---------------------------------------------------------------------

void CPropInventory::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnPropDeactivating);
	UNSUBSCRIBE_EVENT(OnLevelSaving);
	UNSUBSCRIBE_EVENT(OnSOActionDone);

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive()) DisableSI(*pProp);

	Items.Clear();
	CurrWeight = 0.f;
	CurrVolume = 0.f;
}
//---------------------------------------------------------------------

bool CPropInventory::OnPropActivated(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropScriptable>())
	{
		EnableSI(*(CPropScriptable*)pProp);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropInventory::OnPropDeactivating(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropScriptable>())
	{
		DisableSI(*(CPropScriptable*)pProp);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropInventory::OnLevelSaving(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	if (!Items.GetCount())
	{
		GetEntity()->DeleteAttr(CStrID("Inventory"));
		OK;
	}

	// Need to recreate array because else we may rewrite initial level desc in the HRD cache
	Data::PDataArray InvDesc = n_new(Data::CDataArray);
	GetEntity()->SetAttr<Data::PDataArray>(CStrID("Inventory"), InvDesc);

	Data::CData* pData = InvDesc->Reserve(Items.GetCount());
	foreach_stack(Stack, Items)
	{
		n_assert_dbg(Stack->IsValid());

		Data::PParams StackDesc = n_new(Data::CParams(4));

		StackDesc->Set(CStrID("ID"), Stack->GetItemID());
		if (!Stack->GetItem()->IsTemplateInstance())
		{
			Sys::Error("IMPLEMENT ME!!!");
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

bool CPropInventory::OnSOActionDone(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	CStrID ActionID = P->Get<CStrID>(CStrID("Action"));

	if (ActionID == CStrID("PickItem"))
	{
		CStrID SOID = P->Get<CStrID>(CStrID("SO"));
		Game::CEntity* pItemEnt = GameSrv->GetEntityMgr()->GetEntity(SOID);
		CPropItem* pPropItem = pItemEnt ? pItemEnt->GetProperty<CPropItem>() : NULL;
		if (!pPropItem || !pPropItem->Items.IsValid()) OK;

		pPropItem->Items.Remove(AddItem(pPropItem->Items, true));

		if (!pPropItem->Items.IsValid())
		{
			pPropItem->Items.Clear();
			GameSrv->GetEntityMgr()->RequestDestruction(SOID);
		}
	}

	OK;
}
//---------------------------------------------------------------------

U16 CPropInventory::AddItem(Items::PItem NewItem, U16 Count, bool AsManyAsCan)
{
	if (!Count) return 0;

	Items::PItemTpl Tpl = NewItem->GetTpl();
	float TotalW = Tpl->Weight * Count;
	float TotalV = Tpl->Volume * Count;

	float OverW = CurrWeight + TotalW - MaxWeight;
	float OverV = CurrVolume + TotalV - MaxVolume;
	if ((MaxWeight >= 0.f && OverW > 0.f) ||
		(MaxVolume >= 0.f && OverV > 0.f))
	{
		if (AsManyAsCan)
		{
			U16 ExcessCountW = (U16)n_ceil(OverW / Tpl->Weight);
			U16 ExcessCountV = (U16)n_ceil(OverV / Tpl->Volume);
			U16 ExcessCount = n_max(ExcessCountW, ExcessCountV);
			if (Count <= ExcessCount) return 0;
			Count -= ExcessCount;
			TotalW = Tpl->Weight * Count;
			TotalV = Tpl->Volume * Count;
		}
		else
		{
			DBG_ONLY(Sys::Log("CEntity \"%s\": Item \"%s\" is too big or heavy\n", GetEntity()->GetUID(), NewItem->GetID()));
			return 0;
		}
	}

	CurrWeight += TotalW;
	CurrVolume += TotalV;

	foreach_stack(Stack, Items)
		if (Stack->GetItem()->IsEqual(NewItem))
		{
			Stack->Add(Count);
			break;
		}

	if (Stack == Items.End()) Items.Add(Items::CItemStack(NewItem, Count)); //???!!!preallocate & set fields to avoid copying?!

	Data::PParams P = n_new(Data::CParams)(3);
	P->Set(CStrID("Item"), NewItem->GetID());
	P->Set(CStrID("Count"), (int)Count);
	P->Set(CStrID("Entity"), GetEntity()->GetUID());
	EventSrv->FireEvent(CStrID("OnItemAdded"), P);

	return Count;
}
//---------------------------------------------------------------------

U16 CPropInventory::RemoveItem(ItItemStack Stack, U16 Count, bool AsManyAsCan)
{
	if (Stack == Items.End()) FAIL;
	
	//???some flag AllowRemoveEquipped?

	U16 ToRemove;
	if (Stack->GetCount() >= Count) ToRemove = Count;
	else if (AsManyAsCan) ToRemove = Stack->GetCount();
	else return 0;

	CurrWeight -= Stack->GetTpl()->Weight * ToRemove;
	CurrVolume -= Stack->GetVolume();
	
	Data::PParams P = n_new(Data::CParams);
	P->Set(CStrID("Item"), Stack->GetItemID());
	P->Set(CStrID("Count"), (int)ToRemove);
	P->Set(CStrID("Entity"), GetEntity()->GetUID());

	if (Stack->GetCount() > ToRemove)
	{
		Stack->Remove(ToRemove);
		CurrVolume += Stack->GetVolume();
	}
	else Items.Remove(Stack);

	EventSrv->FireEvent(CStrID("OnItemRemoved"), P);

	return ToRemove;
}
//---------------------------------------------------------------------

U16 CPropInventory::RemoveItem(Items::PItem Item, U16 Count, bool AsManyAsCan)
{
	foreach_stack(Stack, Items)
		if (Stack->GetItem()->IsEqual(Item)) break;
	return RemoveItem(Stack, Count, AsManyAsCan) > 0;
}
//---------------------------------------------------------------------

U16 CPropInventory::RemoveItem(CStrID ItemID, U16 Count, bool AsManyAsCan)
{
	U16 LeftToRemove = Count;
	foreach_stack(Stack, Items)
		if (Stack->GetItemID() == ItemID)
		{
			LeftToRemove -= RemoveItem(Stack, LeftToRemove, AsManyAsCan);
			if (!LeftToRemove) return Count;
		}
	return Count - LeftToRemove;
}
//---------------------------------------------------------------------

bool CPropInventory::HasItem(CStrID ItemID, U16 Count)
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

bool CPropInventory::SplitItems(Items::PItem Item, U16 Count, Items::CItemStack& OutStack)
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

	Items.Add(OutStack); //???always append? what about stack-on-cursor? append too?
	OK;
}
//---------------------------------------------------------------------

// Merge all items into one stack
void CPropInventory::MergeItems(Items::PItem Item)
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
			Items.RemoveAt(i);
		}
}
//---------------------------------------------------------------------

}