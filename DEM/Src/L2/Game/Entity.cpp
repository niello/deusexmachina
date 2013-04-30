#include "Entity.h"

#include "Property.h"
#include <Events/EventManager.h>
#include <Loading/EntityFactory.h>

namespace Attr
{
	DeclareAttr(GUID);
}

namespace Game
{
ImplementRTTI(Game::CEntity, Core::CRefCounted);
ImplementFactory(Game::CEntity);

CEntity::CEntity(): CEventDispatcher(32), Flags(ENT_LIVE)
{
}
//---------------------------------------------------------------------

CEntity::~CEntity()
{
	//!!!use virtual GetStorage instead and remove right here!
	EntityFct->DetachAllProperties(*this);
}
//---------------------------------------------------------------------

void CEntity::SetUID(CStrID NewUID)
{
	n_assert(NewUID.IsValid()); //!!!check is valid UID (contains only allowed characters)!
	if (UID == NewUID) return;
	UID = NewUID;
	Set<CStrID>(Attr::GUID, NewUID);
	FireEvent(CStrID("OnEntityRenamed"));
}
//---------------------------------------------------------------------

void CEntity::SetUniqueIDFromAttrTable()
{
	UID = Get<CStrID>(Attr::GUID);
	n_assert(UID.IsValid());
}
//---------------------------------------------------------------------

void CEntity::Activate()
{
	n_assert(IsInactive());
	Flags |= ENT_CHANGING_STATUS;

	GlobalSubscription = EventMgr->Subscribe(NULL, this, &CEntity::OnEvent);

	FireEvent(CStrID("OnEntityActivated"));
	FireEvent(CStrID("OnPropsActivated")); // Needed for initialization of properties dependent on other properties

	Flags |= ENT_ACTIVE;
	Flags &= ~ENT_CHANGING_STATUS;
}
//---------------------------------------------------------------------

void CEntity::Deactivate()
{
	n_assert(IsActive());
	Flags |= ENT_CHANGING_STATUS;

	FireEvent(CStrID("OnEntityDeactivated"));

	GlobalSubscription = NULL;

	Flags &= ~ENT_ACTIVE;
	Flags &= ~ENT_CHANGING_STATUS;
}
//---------------------------------------------------------------------

// OnLoad:
// This method is called after the game world has been loaded from the
// database. At the time when this method is called all entities
// in the world have already been created and their attributes have been
// loaded from the database.
//
// OnStart:
// This method is called in 2 cases:
// - When a level is loaded it is called on all entities after OnLoad when the
//   complete world already exist.
// - When a entity is created at runtime (while a level is active) OnStart is
//   called after the entity is attached to level.

bool CEntity::OnEvent(const CEventBase& Event)
{
	//???event renaming:
	//OnLoad = OnLevel/WorldLoaded!
	//OnStart = OnLevelStart/OnWorldReady? // The world is waiting your arrival x)

	if (IsLive() && ((CEvent&)Event).ID == CStrID("OnBeginFrame"))
		ProcessPendingEvents();

	if (IsLive() ||
		((CEvent&)Event).ID == CStrID("OnLoad") || //!!!???assert UID? GUID was asserted in OnLoad
		((CEvent&)Event).ID == CStrID("OnStart") ||
		((CEvent&)Event).ID == CStrID("OnSave")) //!!!???n_assert(UID.IsValid());
	{
		return !!DispatchEvent(Event);
	}

	//!!!OnBeginFrame, OnMoveBefore, OnMoveAfter, OnRender, OnRenderDebug
	//were checking IsActive() after each property update!
	//mb entity's DispatchEvent should be overridden to achieve this bhv
	//No need to create virtual function, just write new DispatchEvent2
	//and call from above instead of DispatchEvent, cause all other events

	FAIL;
}
//---------------------------------------------------------------------

} // namespace Game
