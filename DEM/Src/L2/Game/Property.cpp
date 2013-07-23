#include "Property.h"

#include <Game/Entity.h>
#include <Events/Subscription.h>

namespace Game
{
__ImplementClassNoFactory(Game::CProperty, Core::CRefCounted);

CProperty::~CProperty()
{
}
//---------------------------------------------------------------------

void CProperty::SetEntity(CEntity* pNewEntity)
{
	if (pNewEntity == pEntity) return;
	bool WasActive = Active;
	if (pEntity)
	{
		if (Active) Deactivate();
		UNSUBSCRIBE_EVENT(OnEntityActivated);
		UNSUBSCRIBE_EVENT(OnEntityDeactivated);
	}
	pEntity = pNewEntity;
	if (pEntity)
	{
		PROP_SUBSCRIBE_PEVENT(OnEntityActivated, CProperty, OnEntityActivated);
		PROP_SUBSCRIBE_PEVENT(OnEntityDeactivated, CProperty, OnEntityDeactivated);
		if (WasActive) Activate();
	}
}
//---------------------------------------------------------------------

void CProperty::Activate()
{
	n_assert_dbg(!Active && pEntity && pEntity->GetLevel());

	Active = InternalActivate();

	if (Active)
	{
		Data::PParams P = n_new(Data::CParams);
		P->Set<PVOID>(CStrID("Prop"), this);
		GetEntity()->FireEvent(CStrID("OnPropActivated"), P);
	}
}
//---------------------------------------------------------------------

void CProperty::Deactivate()
{
	n_assert_dbg(Active && pEntity && pEntity->GetLevel());

	Data::PParams P = n_new(Data::CParams);
	P->Set<PVOID>(CStrID("Prop"), this);
	GetEntity()->FireEvent(CStrID("OnPropDeactivating"), P);

	InternalDeactivate();
	Active = false;

	GetEntity()->FireEvent(CStrID("OnPropDeactivated"), P);
}
//---------------------------------------------------------------------

bool CProperty::OnEntityActivated(const Events::CEventBase& Event)
{
	Activate();
	OK;
}
//---------------------------------------------------------------------

bool CProperty::OnEntityDeactivated(const Events::CEventBase& Event)
{
	if (IsActive()) Deactivate();
	OK;
}
//---------------------------------------------------------------------

}
