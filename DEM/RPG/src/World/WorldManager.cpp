#include "WorldManager.h"

#include <Game/Entity.h>
#include <Events/EventServer.h>
#include <Events/Subscription.h>
#include <Math/Matrix44.h>

namespace RPG
{
RTTI_CLASS_IMPL(RPG::CWorldManager, Core::CObject);
__ImplementSingleton(RPG::CWorldManager);

CWorldManager::CWorldManager()
{
	__ConstructSingleton;

	SUBSCRIBE_PEVENT(OnGameDescLoaded, CWorldManager, OnGameDescLoaded);
	SUBSCRIBE_PEVENT(OnGameSaving, CWorldManager, OnGameSaving);
}
//---------------------------------------------------------------------

CWorldManager::~CWorldManager()
{
	UNSUBSCRIBE_EVENT(OnGameDescLoaded);
	UNSUBSCRIBE_EVENT(OnGameSaving);

	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CWorldManager::OnGameDescLoaded(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	OK;
}
//---------------------------------------------------------------------

bool CWorldManager::OnGameSaving(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	OK;
}
//---------------------------------------------------------------------

//???to L2 GameSrv?
// Level will be set as active externally, because transition can be performed by NPCs too
bool CWorldManager::MakeTransition(const CArray<CStrID>& EntityIDs, CStrID LevelID, CStrID MarkerID, bool UnloadAllLevels)
{
	CArray<Game::PEntity> Entities(EntityIDs.GetCount(), 0);
	for (UPTR i = 0; i < EntityIDs.GetCount(); ++i)
	{
		Game::CEntity* pEnt = nullptr;//GameSrv->GetEntityMgr()->GetEntity(EntityIDs[i]);
		if (pEnt)
		{
			Entities.Add(pEnt);
			pEnt->FireEvent(CStrID("OnLevelSaving"));
			pEnt->Deactivate();
		}
	}

	// May be all entities were already destroyed
	if (!Entities.GetCount()) FAIL;

	if (UnloadAllLevels)
	{
		for (UPTR i = 0; i < Entities.GetCount(); ++i)
			Entities[i]->SetLevel(nullptr);
		//GameSrv->UnloadAllGameLevels();
	}

	//if (!GameSrv->IsLevelLoaded(LevelID) && !GameSrv->LoadGameLevel(LevelID)) FAIL;

	Game::CEntity* pMarker = nullptr;//GameSrv->GetEntityMgr()->GetEntity(MarkerID);
	n_assert(pMarker);

	const matrix44& DestTfm = pMarker->GetAttr<matrix44>(CStrID("Transform"));

	if (Entities.GetCount() > 1)
	{
		// calculate destination, if there is more than one entity
		//!!!need formation to calculate per-entity destinations!
		Sys::Error("IMPLEMENT ME!!! Need formations, if formation is nullptr may auto-position.");
	}
	else Entities[0]->SetAttr<matrix44>(CStrID("Transform"), DestTfm);

	Game::CGameLevel* pNewLevel = nullptr;//GameSrv->GetLevel(LevelID);
	n_assert(pNewLevel);
	for (UPTR i = 0; i < Entities.GetCount(); ++i)
		Entities[i]->SetLevel(pNewLevel);
	for (UPTR i = 0; i < Entities.GetCount(); ++i)
		Entities[i]->Activate();

	OK;
}
//---------------------------------------------------------------------

}
