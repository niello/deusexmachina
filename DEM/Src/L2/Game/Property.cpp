#include "Property.h"

#include <Game/Entity.h>

namespace Game
{
__ImplementClassNoFactory(Game::CProperty, Core::CRefCounted);

IMPL_EVENT_HANDLER_VIRTUAL(OnEntityActivated, CProperty, Activate);
IMPL_EVENT_HANDLER_VIRTUAL(OnEntityDeactivated, CProperty, Deactivate);

CProperty::~CProperty()
{
}
//---------------------------------------------------------------------

void CProperty::SetEntity(CEntity* pNewEntity)
{
	if (pNewEntity == pEntity) return;
	if (pEntity)
	{
		//!!!deactivate if active!
		UNSUBSCRIBE_EVENT(OnEntityActivated);
		UNSUBSCRIBE_EVENT(OnEntityDeactivated);
	}
	pEntity = pNewEntity;
	if (pEntity)
	{
		PROP_SUBSCRIBE_PEVENT(OnEntityActivated, CProperty, ActivateProc);
		PROP_SUBSCRIBE_PEVENT(OnEntityDeactivated, CProperty, DeactivateProc);
		//!!!activate if must be active!
	}
}
//---------------------------------------------------------------------

void CProperty::Activate()
{
	n_assert_dbg(!IsActive());
	Active = true;
}
//---------------------------------------------------------------------

void CProperty::Deactivate()
{
	n_assert_dbg(IsActive());
	Active = false;
}
//---------------------------------------------------------------------

}
