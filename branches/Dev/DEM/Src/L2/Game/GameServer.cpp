#include "GameServer.h"

#include <Game/EntityLoaderCommon.h>
#include <Physics/PhysicsServer.h>
#include <AI/AIServer.h>
#include <Data/DataServer.h>
#include <IO/IOServer.h>
#include <Time/TimeServer.h>
#include <Events/EventManager.h>

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
	ActiveLevel->Trigger();

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
	if (ID != INVALID_INDEX)
	{
		//???update already existing objects or unload the level?
		FAIL;
	}

	PGameLevel Level = n_new(CGameLevel);
	if (!Level->Init(ID, Desc)) FAIL;

	Data::PParams SubDesc;
	if (Desc.Get(SubDesc, CStrID("Entities")))
	{
		for (int i = 0; i < SubDesc->GetCount(); ++i)
		{
			const Data::CParam& EntityPrm = SubDesc->Get(i);
			if (!EntityPrm.IsA<Data::PParams>()) continue;
			Data::PParams EntityDesc = EntityPrm.GetValue<Data::PParams>();

			CStrID LoadingGroup = EntityDesc->Get<CStrID>(CStrID("LoadingGroup"));
			int LoaderIdx = Loaders.FindIndex(LoadingGroup);
			PEntityLoader Loader = (LoaderIdx == INVALID_INDEX) ? DefaultLoader : Loaders.ValueAtIndex(LoaderIdx);
			if (!Loader.IsValid()) continue;

			if (!Loader->Load(EntityPrm.GetName(), Level->GetID(), EntityDesc))
				n_printf("Entity %s not loaded in level %s\n", EntityPrm.GetName().CStr(), Level->GetID().CStr());
		}
	}

	//!!!old and idiotic! use one event, smth like OnLevelLoaded
	//
	//PParams P = n_new(CParams);
	//P->Set(CStrID("DB"), (PVOID)GameDB.GetUnsafe());
	//EventMgr->FireEvent(CStrID("OnLoadBefore"), P);
	//EventMgr->FireEvent(CStrID("OnLoad"), P);
	//EventMgr->FireEvent(CStrID("OnLoadAfter"), P);
	//P = n_new(CParams);
	//P->Set(CStrID("Level"), Level->GetID());
	//EventMgr->FireEvent(CStrID("OnLevelLoaded"), P);

	Levels.Add(Level->GetID(), Level);

	OK;
}
//---------------------------------------------------------------------

void CGameServer::UnloadLevel(CStrID ID)
{
	int LevelIdx = Levels.FindIndex(ID);
	if (ID == INVALID_INDEX) return;

	//???Level->Release()/Term();?

	////!!!unload AI level!
	//GameSrv->Stop();
	//StaticEnvMgr->ClearStaticEnv();
	//EntityMgr->RemoveAllEntities();
	//SceneSrv->RemoveScene(SceneSrv->GetCurrentSceneID());
	//PhysicsSrv->SetLevel(NULL);
	//LevelBox.vmin = vector3::Zero;
	//LevelBox.vmax = vector3::Zero;
	//LevelScript = NULL;
}
//---------------------------------------------------------------------

//!!!maybe merge with LoadGame! load globals, load overrides like quests, load level set in global "Level"
bool CGameServer::StartGame(const nString& FileName)
{
	Data::PParams GameDesc = DataSrv->LoadHRD(FileName);
	if (!GameDesc.IsValid()) FAIL;

	CStrID LevelID = GameDesc->Get<CStrID>(CStrID("Level"));
	Data::PParams LevelDesc = DataSrv->LoadHRD(nString("export:/Levels/") + LevelID.CStr() + ".hrd"); //!!!load PRM!
	if (!LevelDesc.IsValid()) FAIL;

	//!!!load and apply overrides on descs, if there are!

	//!!!HERE:
	//???simply load GameDesc as globals?

	//!!!reset game timers or load, if LoadGame!

	return LoadLevel(LevelID, *LevelDesc);
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

}