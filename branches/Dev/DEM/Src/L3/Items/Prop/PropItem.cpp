#include "PropItem.h"

#include <Items/ItemManager.h>
#include <AI/PropSmartObject.h>
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
		Sys::Error("IMPLEMENT ME!!!");
		//!!!load per-instance fields!
	}
	Items.SetItem(Item);
	Items.SetCount((WORD)GetEntity()->GetAttr<int>(CStrID("ItemCount")));

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

	if (pProp->IsA<CPropSmartObject>())
	{
		((CPropSmartObject*)pProp)->EnableAction(CStrID("PickItem"));
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
		Sys::Error("IMPLEMENT ME!!!");
		//!!!save per-instance fields!
	}
	GetEntity()->SetAttr<int>(CStrID("ItemCount"), (int)Items.GetCount());
	OK;
}
//---------------------------------------------------------------------

}