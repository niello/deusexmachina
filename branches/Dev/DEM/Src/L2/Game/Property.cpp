#include "Property.h"

#include "Entity.h"

namespace Game
{
ImplementRTTI(Game::CProperty, Core::CRefCounted);

CProperty::CProperty(): Active(false), pEntity(NULL)
{
}
//---------------------------------------------------------------------

CProperty::~CProperty()
{
    // n_assert(!this->entity.isvalid());
}
//---------------------------------------------------------------------
//
//bool CProperty::IsActiveInState(const nString& state) const
//{
//	OK;
//}
////---------------------------------------------------------------------

void CProperty::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
}
//---------------------------------------------------------------------

void CProperty::SetEntity(CEntity* pEnt)
{
	n_assert(pEnt && !HasEntity());
	pEntity = pEnt;
	PROP_SUBSCRIBE_PEVENT(OnEntityActivated, CProperty, OnEntityActivated);
	PROP_SUBSCRIBE_PEVENT(OnEntityDeactivated, CProperty, OnEntityDeactivated);

	//???detect if entity is already activated & send OnEntityActivated to it and OnPropAttached to all?
}
//---------------------------------------------------------------------

void CProperty::ClearEntity()
{
	n_assert(pEntity);
	UNSUBSCRIBE_EVENT(OnEntityActivated);
	UNSUBSCRIBE_EVENT(OnEntityDeactivated);
	pEntity = NULL;
}
//---------------------------------------------------------------------

//!!!or can subscribe only instantiating properties to avoid virtual call!
bool CProperty::OnEntityActivated(const CEventBase& Event)
{
	Activate();
	OK;
}
//---------------------------------------------------------------------

//!!!or can subscribe only instantiating properties to avoid virtual call!
bool CProperty::OnEntityDeactivated(const CEventBase& Event)
{
	Deactivate();
	OK;
}
//---------------------------------------------------------------------

void CProperty::Activate()
{
	n_assert(!IsActive());
	Active = true;
}
//---------------------------------------------------------------------

void CProperty::Deactivate()
{
	n_assert(IsActive());
	Active = false;
}
//---------------------------------------------------------------------

}
