#include "GameServer.h"

#include <Game/EntityLoaderCommon.h>
#include <Physics/PhysicsServer.h>
#include <AI/AIServer.h>
#include <Scene/Scene.h>
#include <Physics/PhysicsLevel.h>
#include <Data/DataServer.h>
#include <IO/IOServer.h>
#include <Time/TimeServer.h>
#include <Scripting/ScriptObject.h>
#include <Events/EventManager.h>

/* User profile stuff:

//!!!must store settings!

IOSrv->CreateDirectory("appdata:Profiles/Default");

inline nString CLoaderServer::GetDatabasePath() const
{
	return "appdata:profiles/default/" + GameDBName + ".db3";
}
//---------------------------------------------------------------------

// Returns the path to the user's savegame directory (inside the profile
// directory) using the Nebula2 filesystem path conventions.
inline nString CLoaderServer::GetSaveGameDirectory() const
{
	return "appdata:profiles/default/save";
}
//---------------------------------------------------------------------

// Get the complete filename to a savegame file.
inline nString CLoaderServer::GetSaveGamePath(const nString& SaveGameName) const
{
	return GetSaveGameDirectory() + "/" + SaveGameName + ".db3";
}
//---------------------------------------------------------------------

*/
//???when and how to load static data (former static.db3)? or it is just a set of descs now?

/* Savegame stuff:
	PParams P = n_new(CParams);
	P->Set(CStrID("DB"), (PVOID)GameDB.GetUnsafe());
	EventMgr->FireEvent(CStrID("OnSaveBefore"), P);
	EventMgr->FireEvent(CStrID("OnSave"), P);
	EventMgr->FireEvent(CStrID("OnSaveAfter"), P);
	SaveGlobalAttributes();
	//SaveEntities();
*/

namespace Game
{
__ImplementClassNoFactory(Game::CGameServer, Core::CRefCounted);
__ImplementSingleton(CGameServer);

CGameServer::CGameServer(): IsOpen(false)
{
	__ConstructSingleton;
	IOSrv->SetAssign("iao", "game:iao");
}
//---------------------------------------------------------------------

bool CGameServer::Open()
{
	n_assert(!IsOpen);

	GameTimeSrc = n_new(Time::CTimeSource);
	TimeSrv->AttachTimeSource(CStrID("Game"), GameTimeSrc);

	EntityManager = n_new(CEntityManager);
	EntityManager->Open();

	if (!DefaultLoader.IsValid()) DefaultLoader = n_new(CEntityLoaderCommon);

	IsOpen = true;
	OK;
}
//---------------------------------------------------------------------

void CGameServer::Close()
{
	n_assert(IsOpen);

	TimeSrv->RemoveTimeSource(CStrID("Game"));
	GameTimeSrc = NULL;

	EntityManager->Close();
	EntityManager = NULL;

	IsOpen = false;
}
//---------------------------------------------------------------------

void CGameServer::Trigger()
{
	EventMgr->FireEvent(CStrID("OnBeginFrame"));

	AISrv->Trigger(); // Pathfinding queries inside

	//!!!trigger all levels, but send to the audio, video, scene and debug rendering only data from the active level!
	if (ActiveLevel.IsValid()) ActiveLevel->Trigger();

	EventMgr->FireEvent(CStrID("OnEndFrame"));
}
//---------------------------------------------------------------------

// If group loader exists and is set to NULL, group will be skipped
void CGameServer::SetEntityLoader(CStrID Group, PEntityLoader Loader)
{
	if (Group.IsValid()) Loaders.Set(Group, Loader);
	else DefaultLoader = Loader;
}
//---------------------------------------------------------------------

void CGameServer::ClearEntityLoader(CStrID Group)
{
	if (Group.IsValid()) Loaders.Erase(Group);
	else DefaultLoader = NULL; //???allow?
}
//---------------------------------------------------------------------

bool CGameServer::LoadLevel(CStrID ID, const Data::CParams& Desc)
{
	int LevelIdx = Levels.FindIndex(ID);
	if (LevelIdx != INVALID_INDEX)
	{
		//???update already existing objects or unload the level?
		FAIL;
	}

	Data::PParams P = n_new(Data::CParams);
	P->Set(CStrID("ID"), ID);
	EventMgr->FireEvent(CStrID("OnLevelLoading"), P); //???or after a level is added, but entities aren't loaded?

	PGameLevel Level = n_new(CGameLevel);
	if (!Level->Init(ID, Desc)) FAIL;

	Levels.Add(Level->GetID(), Level);

	Data::PParams SubDesc;
	if (Desc.Get(SubDesc, CStrID("Entities")))
	{
		Level->FireEvent(CStrID("OnEntitiesLoading"));

		for (int i = 0; i < SubDesc->GetCount(); ++i)
		{
			const Data::CParam& EntityPrm = SubDesc->Get(i);
			if (!EntityPrm.IsA<Data::PParams>()) continue;
			Data::PParams EntityDesc = EntityPrm.GetValue<Data::PParams>();

			CStrID LoadingGroup = EntityDesc->Get<CStrID>(CStrID("LoadingGroup"), CStrID::Empty);
			int LoaderIdx = Loaders.FindIndex(LoadingGroup);
			PEntityLoader Loader = (LoaderIdx == INVALID_INDEX) ? DefaultLoader : Loaders.ValueAtIndex(LoaderIdx);
			if (!Loader.IsValid()) continue;

			if (!Loader->Load(EntityPrm.GetName(), *Level, EntityDesc))
				n_printf("Entity %s not loaded in level %s, group is %s\n",
					EntityPrm.GetName().CStr(), Level->GetID().CStr(), LoadingGroup.CStr());
		}

		Level->FireEvent(CStrID("OnEntitiesLoaded"));
	}

	EventMgr->FireEvent(CStrID("OnLevelLoaded"), P);

	OK;
}
//---------------------------------------------------------------------

void CGameServer::UnloadLevel(CStrID ID)
{
	int LevelIdx = Levels.FindIndex(ID);
	if (ID == INVALID_INDEX) return;

	PGameLevel Level = Levels.ValueAtIndex(LevelIdx);

	//!!!if in game mode, save diff of this level to a continue set!
	// re-entering location LoadLevel call will load continue diff
	// If mode is not a game, but is a simple level, we don't need to save diff,
	// moreover, we have no user profile

	if (ActiveLevel == Level) SetActiveLevel(CStrID::Empty);

	Data::PParams P = n_new(Data::CParams);
	P->Set(CStrID("ID"), ID);
	EventMgr->FireEvent(CStrID("OnLevelUnloading"), P);

	Level->FireEvent(CStrID("OnEntitiesUnloading"));
	EntityMgr->DeleteEntities(*Level);
	//!!!delete static environment objects! //StaticEnvMgr->ClearStaticEnv();
	Level->FireEvent(CStrID("OnEntitiesUnloaded"));

	Levels.EraseAt(LevelIdx);

	Level->Term();

	EventMgr->FireEvent(CStrID("OnLevelUnloaded"), P); //???or before a level is removed, but entities are unloaded?

	n_assert_dbg(Level->GetRefCount() == 1);
}
//---------------------------------------------------------------------

//!!!maybe merge with LoadGame! load globals, load overrides like quests, load level set in global "Level"
bool CGameServer::StartGame(const nString& FileName)
{
	Data::PParams GameDesc = DataSrv->LoadHRD(FileName);
	if (!GameDesc.IsValid()) FAIL;

	//!!!if there is GameDesc override, apply it here!

	for (int i = 0; i < GameDesc->GetCount(); ++i)
		SetGlobalAttr(GameDesc->Get(i).GetName(), GameDesc->Get(i).GetRawValue());

	CStrID LevelID = GetGlobalAttr<CStrID>(CStrID("ActiveLevel"));
	Data::PParams LevelDesc = DataSrv->LoadHRD(nString("export:Game/Levels/") + LevelID.CStr() + "/Level.hrd"); //!!!load PRM!
	if (!LevelDesc.IsValid())
	{
		Attrs.Clear();
		FAIL;
	}

	//!!!if there is LevelDesc override, apply it here!

	//!!!HERE:
	//???simply load GameDesc as globals?

	//!!!reset game timers or load, if LoadGame!

	return LoadLevel(LevelID, *LevelDesc) && SetActiveLevel(LevelID);
}
//---------------------------------------------------------------------

void CGameServer::PauseGame(bool Pause) const
{
	if (Pause)
	{
		GameTimeSrc->Pause();
		EventMgr->FireEvent(CStrID("OnGamePaused"));
	}
	else
	{
		GameTimeSrc->Unpause();
		EventMgr->FireEvent(CStrID("OnGameUnpaused"));
	}
}
//---------------------------------------------------------------------

/*
bool CEnvQueryManager::OnFrame(const Events::CEventBase& Event)
{
	CStrID OldEntityUnderMouse = EntityUnderMouse;

    // Get 3d contact under mouse
    if (!UISrv->IsMouseOverGUI())
	{
		float XRel, YRel;
		InputSrv->GetMousePosRel(XRel, YRel);
		line3 Ray;
		SceneSrv->GetCurrentScene()->GetMainCamera()->GetRay3D(XRel, YRel, 5000.f, Ray);
		const Physics::CContactPoint* pContact = PhysicsSrv->GetClosestContactAlongRay(Ray.start(), Ray.vec());
        MouseIntersection = (pContact != NULL);
        if (MouseIntersection)
        {
            // Store intersection position
            MousePos3D = pContact->Position;
            UpVector = pContact->UpVector;

            // Get entity under mouse
            Physics::CEntity* pPhysEntity = PhysicsSrv->FindEntityByUniqueID(pContact->EntityID);
			EntityUnderMouse = (pPhysEntity) ? pPhysEntity->GetUserData() : CStrID::Empty;
        }
		else
		{
			// Reset values
			EntityUnderMouse = CStrID::Empty;
			MousePos3D.set(0.0f, 0.0f, 0.0f);
		}
    }
	else
	{
		MouseIntersection = false;
		EntityUnderMouse = CStrID::Empty;
		MousePos3D.set(0.0f, 0.0f, 0.0f);
	}

	if (OldEntityUnderMouse != EntityUnderMouse)
	{
		Game::CEntity* pEntityUnderMouse = EntityMgr->GetEntity(OldEntityUnderMouse);
		if (pEntityUnderMouse)
		{
			PParams P = n_new(CParams);
			P->Set(CStrID("IsOver"), false);
			pEntityUnderMouse->FireEvent(CStrID("ObjMouseOver"), P);
		}
		
		pEntityUnderMouse = GetEntityUnderMouse();
		if (pEntityUnderMouse)
		{
			PParams P = n_new(CParams);
			P->Set(CStrID("IsOver"), true);
			pEntityUnderMouse->FireEvent(CStrID("ObjMouseOver"), P);
		}
	}

	OK;
}
//---------------------------------------------------------------------
*/

}