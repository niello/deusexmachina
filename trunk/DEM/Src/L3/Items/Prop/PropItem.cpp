#include "PropItem.h"

#include <Items/ItemAttrs.h>
#include <Items/ItemManager.h>
#include <Items/Prop/PropInventory.h>
#include <Game/Mgr/EntityManager.h>
#include <Loading/EntityFactory.h>

namespace Properties
{
ImplementRTTI(Properties::CPropItem, Game::CProperty);
ImplementFactory(Properties::CPropItem);
ImplementPropertyStorage(CPropItem, 128);
RegisterProperty(CPropItem);

using namespace Items;

void CPropItem::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	Attrs.Append(Attr::ItemTplID);
	Attrs.Append(Attr::ItemInstID);
	Attrs.Append(Attr::ItemCount);
}
//---------------------------------------------------------------------

void CPropItem::Activate()
{
	Game::CProperty::Activate();

	PItem Item =
		ItemMgr->GetItemTpl(GetEntity()->Get<CStrID>(Attr::ItemTplID))->GetTemplateItem();
	int InstID = GetEntity()->Get<int>(Attr::ItemInstID);
	if (InstID > -1)
	{
		Item = Item->Clone();
		//!!!Item->LoadFromDB(InstID);!
	}
	Items.SetItem(Item);
	Items.SetCount((WORD)GetEntity()->Get<int>(Attr::ItemCount));
	
	PROP_SUBSCRIBE_PEVENT(OnSave, CPropItem, OnSave);
	PROP_SUBSCRIBE_PEVENT(OnPropsActivated, CPropItem, OnPropsActivated);
	PROP_SUBSCRIBE_PEVENT(PickItem, CPropItem, OnPickItem);
}
//---------------------------------------------------------------------

void CPropItem::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnSave);
	UNSUBSCRIBE_EVENT(OnPropsActivated);
	UNSUBSCRIBE_EVENT(PickItem);
	Items.Clear();
	Game::CProperty::Deactivate();
}
//---------------------------------------------------------------------

bool CPropItem::OnSave(const Events::CEventBase& Event)
{
	//???store smth like bool Changed?
	if (Items.IsValid()) //???assert? or DeleteEntity(this) if !valid?
	{
		GetEntity()->Set<CStrID>(Attr::ItemTplID, Items.GetItemID());
		if (Items.GetItem()->IsTemplateInstance())
			GetEntity()->Set<int>(Attr::ItemInstID, -1);
		else
		{
			n_assert(false);
			//!!!save instance, get its ID & save ID!
		}
		GetEntity()->Set<int>(Attr::ItemCount, (int)Items.GetCount());
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

		Game::CEntity* pActorEnt = EntityMgr->GetEntityByID(P->Get<CStrID>(CStrID("Actor")));
		CPropInventory* pInv = pActorEnt ? pActorEnt->FindProperty<CPropInventory>() : NULL;
		if (pInv)
		{
			pInv->AddItem(Items);
			Items.Clear();
			EntityMgr->DeleteEntity(GetEntity());
		}
	}

	OK;
}
//---------------------------------------------------------------------

} // namespace Properties
