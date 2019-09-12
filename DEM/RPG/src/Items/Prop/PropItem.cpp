#include "PropItem.h"

#include <Items/ItemManager.h>
#include <AI/PropSmartObject.h>
#include <UI/PropUIControl.h>
#include <Game/Entity.h>
#include <Events/Subscription.h>
#include <Core/Factory.h>

namespace Prop
{
__ImplementClass(Prop::CPropItem, 'PITM', Game::CProperty);
__ImplementPropertyStorage(CPropItem);

CPropItem::CPropItem() {}
CPropItem::~CPropItem() {}

bool CPropItem::InternalActivate()
{
	Items::PItem Item = ItemMgr->GetItemTpl(GetEntity()->GetAttr<CStrID>(CStrID("ItemTplID")))->GetTemplateItem();
	Data::PParams ItemInst = GetEntity()->GetAttr<Data::PParams>(CStrID("ItemInstance"), nullptr);
	if (ItemInst.IsValidPtr())
	{
		Item = Item->Clone();
		Sys::Error("IMPLEMENT ME!!!");
		//!!!load per-instance fields!
	}
	Items.SetItem(Item);
	Items.SetCount((U16)GetEntity()->GetAttr<int>(CStrID("ItemCount")));

	CPropUIControl* pPropUI = GetEntity()->GetProperty<CPropUIControl>();
	if (pPropUI && pPropUI->IsActive()) pPropUI->SetUIName(Items.GetTpl()->UIName);

	CPropSmartObject* pPropSO = GetEntity()->GetProperty<CPropSmartObject>();
	if (pPropSO && pPropSO->IsActive()) pPropSO->EnableAction(CStrID("PickItem"));

	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropItem, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnLevelSaving, CPropItem, OnLevelSaving);

	OK;
}
//---------------------------------------------------------------------

void CPropItem::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnLevelSaving);

	CPropSmartObject* pPropSO = GetEntity()->GetProperty<CPropSmartObject>();
	if (pPropSO && pPropSO->IsActive()) pPropSO->EnableAction(CStrID("PickItem"), false);

	Items.Clear();
}
//---------------------------------------------------------------------

bool CPropItem::OnPropActivated(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropUIControl>())
	{
		((CPropUIControl*)pProp)->SetUIName(Items.GetTpl()->UIName);
		OK;
	}

	if (pProp->IsA<CPropSmartObject>())
	{
		((CPropSmartObject*)pProp)->EnableAction(CStrID("PickItem"));
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropItem::OnLevelSaving(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	if (!Items.IsValid()) OK;
	GetEntity()->SetAttr<CStrID>(CStrID("ItemTplID"), Items.GetItemID());
	if (!Items.GetItem()->IsTemplateInstance())
	{
		Sys::Error("IMPLEMENT ME!!!");
		//!!!save per-instance fields!
	}
	GetEntity()->SetAttr<int>(CStrID("ItemCount"), (int)Items.GetCount());
	OK;
}
//---------------------------------------------------------------------

}