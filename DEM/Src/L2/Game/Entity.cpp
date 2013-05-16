#include "Entity.h"

#include "Property.h"
#include <Game/GameLevel.h>
#include <Events/EventManager.h>

namespace Game
{
__ImplementClassNoFactory(Game::CEntity, Core::CRefCounted);

CEntity::CEntity(CStrID _UID, CGameLevel& _Level): CEventDispatcher(16), UID(_UID), Level(&_Level)
{
}
//---------------------------------------------------------------------

//CEntity::~CEntity()
//{
//}
////---------------------------------------------------------------------

void CEntity::SetUID(CStrID NewUID)
{
	n_assert(NewUID.IsValid());
	if (UID == NewUID) return;
	UID = NewUID;
}
//---------------------------------------------------------------------

void CEntity::Activate()
{
	n_assert(IsInactive());
	Flags.Set(ChangingActivity);

	GlobalSub = EventMgr->Subscribe(NULL, this, &CEntity::OnEvent);

	FireEvent(CStrID("OnEntityActivated"));
	FireEvent(CStrID("OnPropsActivated")); // Needed for initialization of properties dependent on other properties

	Flags.Set(Active);
	Flags.Clear(ChangingActivity);
}
//---------------------------------------------------------------------

void CEntity::Deactivate()
{
	n_assert(IsActive());
	Flags.Set(ChangingActivity);

	FireEvent(CStrID("OnEntityDeactivated"));

	GlobalSub = NULL;

	Flags.Clear(Active | ChangingActivity);
}
//---------------------------------------------------------------------

bool CEntity::OnEvent(const Events::CEventBase& Event)
{
	if (((Events::CEvent&)Event).ID == CStrID("OnBeginFrame")) ProcessPendingEvents();
	return !!DispatchEvent(Event);
}
//---------------------------------------------------------------------

} // namespace Game
