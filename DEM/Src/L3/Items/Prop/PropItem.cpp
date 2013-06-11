#include "PropItem.h"

#include <Items/ItemManager.h>
#include <Items/Prop/PropInventory.h>
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
		//!!!init from params!
	}
	Items.SetItem(Item);
	Items.SetCount((WORD)GetEntity()->GetAttr<int>(CStrID("ItemCount")));
	
	PROP_SUBSCRIBE_PEVENT(OnSave, CPropItem, OnSave);
	PROP_SUBSCRIBE_PEVENT(OnPropsActivated, CPropItem, OnPropsActivated);
	PROP_SUBSCRIBE_PEVENT(PickItem, CPropItem, OnPickItem);

	OK;
}
//---------------------------------------------------------------------

void CPropItem::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(OnSave);
	UNSUBSCRIBE_EVENT(OnPropsActivated);
	UNSUBSCRIBE_EVENT(PickItem);
	Items.Clear();
}
//---------------------------------------------------------------------

bool CPropItem::OnSave(const Events::CEventBase& Event)
{
	//???store smth like bool Changed?
	if (Items.IsValid()) //???assert? or DeleteEntity(this) if !valid?
	{
		GetEntity()->SetAttr<CStrID>(CStrID("ItemTplID"), Items.GetItemID());
		if (!Items.GetItem()->IsTemplateInstance())
		{
			n_assert(false);
			//!!!save instance desc!
		}
		GetEntity()->SetAttr<int>(CStrID("ItemCount"), (int)Items.GetCount());
	}
	OK;
}
//---------------------------------------------------------------------

bool CPropItem::OnPropsActivated(const Events::CEventBase& Event)
{
	if (Items.IsValid() && Items.GetTpl()->UIName.IsValid())
	{
		PParams P = n_new(CParams);
		P->Set(CStrID("Text"), Items.GetTpl()->UIName);
		GetEntity()->FireEvent(CStrID("OverrideUIName"), P);
	}
	OK;
}
//---------------------------------------------------------------------

// "PickItem" command handler, actual item picking is here
bool CPropItem::OnPickItem(const Events::CEventBase& Event)
{
	if (Items.IsValid())
	{
		PParams P = ((const Events::CEvent&)Event).Params;

		Game::CEntity* pActorEnt = EntityMgr->GetEntity(P->Get<CStrID>(CStrID("Actor")));
		CPropInventory* pInv = pActorEnt ? pActorEnt->GetProperty<CPropInventory>() : NULL;
		if (pInv)
		{
			pInv->AddItem(Items);
			Items.Clear();
			EntityMgr->DeleteEntity(*GetEntity()); //!!!check deletion from itself!
		}
	}

	OK;
}
//---------------------------------------------------------------------

} // namespace Prop
