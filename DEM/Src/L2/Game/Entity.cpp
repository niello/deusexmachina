#include "Entity.h"

#include "Property.h"
#include <Game/GameLevel.h>
#include <Events/EventManager.h>

namespace Game
{
__ImplementClassNoFactory(Game::CEntity, Core::CRefCounted);

CEntity::CEntity(CStrID _UID, CGameLevel& _Level): CEventDispatcher(16), UID(_UID), Level(&_Level)
{
	LevelSub = Level->Subscribe(NULL, this, &CEntity::OnEvent);
}
//---------------------------------------------------------------------

CEntity::~CEntity()
{
	n_assert_dbg(IsInactive());
	LevelSub = NULL;
}
//---------------------------------------------------------------------

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

	Flags.Clear(Active | ChangingActivity);
}
//---------------------------------------------------------------------

bool CEntity::OnEvent(const Events::CEventBase& Event)
{
	CStrID EvID = ((Events::CEvent&)Event).ID;

	if (EvID == CStrID("OnEntitiesLoaded"))
	{
		Activate();
		OK;
	}

	if (EvID == CStrID("OnEntitiesUnloading"))
	{
		Deactivate();
		OK;
	}

	if (EvID == CStrID("OnBeginFrame")) ProcessPendingEvents();
	return !!DispatchEvent(Event);
}
//---------------------------------------------------------------------

} // namespace Game
