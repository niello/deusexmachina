#include "WorldManager.h"

#include <Game/GameServer.h>
#include <Events/EventServer.h>

namespace RPG
{
__ImplementClassNoFactory(RPG::CWorldManager, Core::CRefCounted);
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

bool CWorldManager::OnGameDescLoaded(const Events::CEventBase& Event)
{
	OK;
}
//---------------------------------------------------------------------

bool CWorldManager::OnGameSaving(const Events::CEventBase& Event)
{
	OK;
}
//---------------------------------------------------------------------

// Level will be set as active externally, because here NPC may walk
bool CWorldManager::MakeTransition(const CArray<CStrID>& EntityIDs, CStrID DepartureLevelID, CStrID DestLevelID, CStrID DestMarkerID, bool UnloadDepartureLevel)
{
	CArray<Game::PEntity> Entities(EntityIDs.GetCount(), 0);
	for (int i = 0; i < EntityIDs.GetCount(); ++i)
	{
		Game::CEntity* pEnt = EntityMgr->GetEntity(EntityIDs[i]);
		if (pEnt)
		{
			n_assert_dbg(pEnt->GetLevel().GetID() == DepartureLevelID);
			Entities.Add(pEnt);
			pEnt->Deactivate();
		}
	}

	// May be all entities were already destroyed
	if (!Entities.GetCount()) FAIL;

	//!!!may need to unload several levels (or always all?)
	if (UnloadDepartureLevel)
	{
		for (int i = 0; i < Entities.GetCount(); ++i)
			Entities[i]->SetLevel(NULL);
		GameSrv->UnloadGameLevel(DepartureLevelID);
	}

	if (!GameSrv->IsLevelLoaded(DestLevelID) && !GameSrv->LoadGameLevel(DestLevelID)) FAIL;

	//!!!if (UnloadDepartureLevel) UnloadUnreferencedResources();

	Game::CEntity* pMarker = EntityMgr->GetEntity(DestMarkerID);
	n_assert(pMarker);

	const matrix44& DestTfm = pMarker->GetAttr<matrix44>(CStrID("Transform"));

	if (Entities.GetCount() > 1)
	{
		// calculate destination, if there is more than one entity
		//!!!need formation to calculate per-entity destinations!
		n_error("IMPLEMENT ME!!! Need formations, if formation is NULL may auto-position.");
	}
	else Entities[0]->SetAttr<matrix44>(CStrID("Transform"), DestTfm);

	Game::CGameLevel* pNewLevel = GameSrv->GetLevel(DestLevelID);
	n_assert(pNewLevel);
	for (int i = 0; i < Entities.GetCount(); ++i)
		Entities[i]->SetLevel(pNewLevel);
	for (int i = 0; i < Entities.GetCount(); ++i)
		Entities[i]->Activate();

	OK;
}
//---------------------------------------------------------------------

}