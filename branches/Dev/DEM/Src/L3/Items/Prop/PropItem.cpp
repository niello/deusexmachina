#include "PropItem.h"

#include <Items/ItemManager.h>
#include <Items/Prop/PropInventory.h>
#include <UI/PropUIControl.h>
#include <Game/EntityManager.h>

namespace Prop
{
__ImplementClass(Prop::CPropItem, 'PITM', Game::CProperty);
__ImplementPropertyStorage(CPropItem);

using namespace Items;

bool CPropItem::InternalActivate()
{
	PItem Item = ItemMgr->GetItemTpl(GetEntity()->GetAttr<CStrID>(CStrID("ItemTplID")))->GetTemplateItem();
	Data::PParams ItemInst = GetEntity()->GetAttr<Data::PParams>(CStrID("ItemInstance"), NULL);
	if (ItemInst.IsValid())
	{
		Item = Item->Clone();
		n_error("IMPLEMENT ME!!!");
		//!!!load per-instance fields!
	}
	Items.SetItem(Item);
	Items.SetCount((WORD)GetEntity()->GetAttr<int>(CStrID("ItemCount")));

	CPropUIControl* pProp = GetEntity()->GetProperty<CPropUIControl>();
	if (pProp && pProp->IsActive())
		pProp->SetUIName(Items.GetTpl()->UIName);
	
	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropItem, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnLevelSaving, CPropItem, OnLevelSaving);
	PROP_SUBSCRIBE_PEVENT(PickItem, CPropItem, OnPickItem);

	OK;
}
//---------------------------------------------------------------------

void CPropItem::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnLevelSaving);
	UNSUBSCRIBE_EVENT(PickItem);
	Items.Clear();
}
//---------------------------------------------------------------------

bool CPropItem::OnPropActivated(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropUIControl>())
	{
		((CPropUIControl*)pProp)->SetUIName(Items.GetTpl()->UIName);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropItem::OnLevelSaving(const Events::CEventBase& Event)
{
	if (!Items.IsValid()) OK;
	GetEntity()->SetAttr<CStrID>(CStrID("ItemTplID"), Items.GetItemID());
	if (!Items.GetItem()->IsTemplateInstance())
	{
		n_error("IMPLEMENT ME!!!");
		//!!!save per-instance fields!
	}
	GetEntity()->SetAttr<int>(CStrID("ItemCount"), (int)Items.GetCount());
	OK;
}
//---------------------------------------------------------------------

// "PickItem" command handler, actual item picking is here
bool CPropItem::OnPickItem(const Events::CEventBase& Event)
{
	if (!Items.IsValid()) OK;

	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CEntity* pActorEnt = EntityMgr->GetEntity(P->Get<CStrID>(CStrID("Actor")));
	CPropInventory* pInv = pActorEnt ? pActorEnt->GetProperty<CPropInventory>() : NULL;
	if (pInv)
	{
		//!!!can add only part of the stack (check weight and volume)!
		//kill entity only when empty. or deactivate prop?
		pInv->AddItem(Items);
		Items.Clear();
		EntityMgr->RequestDestruction(GetEntity()->GetUID());
	}

	OK;
}
//---------------------------------------------------------------------

}