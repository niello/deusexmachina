#include "GameServer.h"

#include <Game/EntityLoaderCommon.h>
#include <AI/AIServer.h>
#include <Scene/Scene.h>
#include <Physics/PhysicsWorld.h>
#include <IO/IOServer.h>
#include <IO/FSBrowser.h>
#include <UI/UIServer.h>
#include <Input/InputServer.h>
#include <Time/TimeServer.h>
#include <Scripting/ScriptObject.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <Events/EventServer.h>

namespace Game
{
__ImplementClassNoFactory(Game::CGameServer, Core::CRefCounted);
__ImplementSingleton(CGameServer);

bool CGameServer::Open()
{
	n_assert(!IsOpen);

	GameTimeSrc = n_new(Time::CTimeSource);
	TimeSrv->AttachTimeSource(CStrID("Game"), GameTimeSrc);

	EntityManager = n_new(CEntityManager);
	StaticEnvManager = n_new(CStaticEnvManager);

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

	StaticEnvManager = NULL;
	EntityManager = NULL;

	IsOpen = false;
}
//---------------------------------------------------------------------

void CGameServer::Trigger()
{
	EntityMgr->DeferredDeleteEntities();

	UpdateMouseIntersectionInfo();

	EventSrv->FireEvent(CStrID("OnBeginFrame"));

	AISrv->Trigger(); // Pathfinding queries inside

	//!!!trigger all levels, but send to the audio, video, scene and debug rendering only data from the active level!
	if (ActiveLevel.IsValid()) ActiveLevel->Trigger();

	EventSrv->FireEvent(CStrID("OnEndFrame"));
}
//---------------------------------------------------------------------

void CGameServer::UpdateMouseIntersectionInfo()
{
	CStrID OldEntityUnderMouse = EntityUnderMouse;

	if (UISrv->IsMouseOverGUI() || !ActiveLevel.IsValid()) HasMouseIsect = false;
	else
	{
		float XRel, YRel;
		InputSrv->GetMousePosRel(XRel, YRel);
		HasMouseIsect = ActiveLevel->GetIntersectionAtScreenPos(XRel, YRel, &MousePos3D, &EntityUnderMouse);
	}

	if (!HasMouseIsect)
	{
		EntityUnderMouse = CStrID::Empty;
		MousePos3D.set(0.0f, 0.0f, 0.0f);
	}

	if (OldEntityUnderMouse != EntityUnderMouse)
	{
		Game::CEntity* pEntityUnderMouse = EntityMgr->GetEntity(OldEntityUnderMouse);
		if (pEntityUnderMouse) pEntityUnderMouse->FireEvent(CStrID("OnMouseLeave"));
		pEntityUnderMouse = GetEntityUnderMouse();
		if (pEntityUnderMouse) pEntityUnderMouse->FireEvent(CStrID("OnMouseEnter"));
	}
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
	if (Group.IsValid()) Loaders.Remove(Group);
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
	EventSrv->FireEvent(CStrID("OnLevelLoading"), P); //???or after a level is added, but entities aren't loaded?

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
			PEntityLoader Loader = (LoaderIdx == INVALID_INDEX) ? DefaultLoader : Loaders.ValueAt(LoaderIdx);
			if (!Loader.IsValid()) continue;

			if (!Loader->Load(EntityPrm.GetName(), *Level, EntityDesc))
				n_printf("Entity %s not loaded in level %s, group is %s\n",
					EntityPrm.GetName().CStr(), Level->GetID().CStr(), LoadingGroup.CStr());
		}

		Level->FireEvent(CStrID("OnEntitiesLoaded"));
	}

	Data::PDataArray SelArray;
	if (Desc.Get(SelArray, CStrID("SelectedEntities")))
		for (int i = 0; i < SelArray->GetCount(); ++i)
			Level->AddToSelection(SelArray->Get<CStrID>(i));

	EventSrv->FireEvent(CStrID("OnLevelLoaded"), P);

	OK;
}
//---------------------------------------------------------------------

void CGameServer::UnloadLevel(CStrID ID)
{
	int LevelIdx = Levels.FindIndex(ID);
	if (ID == INVALID_INDEX) return;

	PGameLevel Level = Levels.ValueAt(LevelIdx);

	if (ActiveLevel == Level) SetActiveLevel(CStrID::Empty);

	Data::PParams P = n_new(Data::CParams);
	P->Set(CStrID("ID"), ID);
	EventSrv->FireEvent(CStrID("OnLevelUnloading"), P);

	Level->FireEvent(CStrID("OnEntitiesUnloading"));
	EntityMgr->DeleteEntities(*Level);
	StaticEnvMgr->DeleteStaticObjects(*Level);
	Level->FireEvent(CStrID("OnEntitiesUnloaded"));

	Levels.RemoveAt(LevelIdx);

	Level->Term();

	EventSrv->FireEvent(CStrID("OnLevelUnloaded"), P); //???or before a level is removed, but entities are unloaded?

	n_assert_dbg(Level->GetRefCount() == 1);
}
//---------------------------------------------------------------------

bool CGameServer::SetActiveLevel(CStrID ID)
{
	PGameLevel NewLevel;
	if (ID.IsValid())
	{
		int LevelIdx = Levels.FindIndex(ID);
		if (LevelIdx == INVALID_INDEX) FAIL;
		NewLevel = Levels.ValueAt(LevelIdx);
	}

	if (NewLevel != ActiveLevel)
	{
		EventSrv->FireEvent(CStrID("OnActiveLevelChanging"));
		ActiveLevel = NewLevel;
		SetGlobalAttr<CStrID>(CStrID("ActiveLevel"), ActiveLevel.IsValid() ? ID : CStrID::Empty);

		EntityUnderMouse = CStrID::Empty;
		HasMouseIsect = false;
		UpdateMouseIntersectionInfo();

		EventSrv->FireEvent(CStrID("OnActiveLevelChanged"));
	}

	OK;
}
//---------------------------------------------------------------------

bool CGameServer::ValidateLevel(CGameLevel& Level)
{
	bool Result = !Level.GetScene() || Level.GetScene()->GetRootNode().ValidateResources();

	//!!!???activate entities here and not in level loading?!

	Data::PParams P = n_new(Data::CParams(1));
	P->Set(CStrID("ID"), Level.GetID());
	EventSrv->FireEvent(CStrID("OnLevelValidated"), P);
	return Result;
}
//---------------------------------------------------------------------

void CGameServer::EnumProfiles(CArray<CString>& Out) const
{
	Out.Clear();

	CString ProfilesDir = "AppData:Profiles";
	if (!IOSrv->DirectoryExists(ProfilesDir))
	{
		IOSrv->CreateDirectory(ProfilesDir);
		return;
	}

	IO::CFSBrowser Browser;
	Browser.SetAbsolutePath(ProfilesDir);
	if (!Browser.IsCurrDirEmpty()) do
	{
		if (Browser.IsCurrEntryDir()) Out.Add(Browser.GetCurrEntryName());
	}
	while (Browser.NextCurrDirEntry());
}
//---------------------------------------------------------------------

bool CGameServer::CreateProfile(const CString& Name) const
{
	CString ProfileDir = "AppData:Profiles/" + Name;
	if (IOSrv->DirectoryExists(ProfileDir)) FAIL;
	return IOSrv->CreateDirectory(ProfileDir + "/Saves") &&
		IOSrv->CreateDirectory(ProfileDir + "/Continue/Levels");
}
//---------------------------------------------------------------------

bool CGameServer::DeleteProfile(const CString& Name) const
{
	if (CurrProfile == Name && GameFileName.IsValid()) FAIL; //???or stop game and set current to empty?
	return IOSrv->DeleteDirectory("AppData:Profiles/" + Name);
}
//---------------------------------------------------------------------

bool CGameServer::SetCurrentProfile(const CString& Name)
{
	CurrProfile = Name;

	//!!!load and apply settings!

	OK;
}
//---------------------------------------------------------------------

//!!!pack saves to files! single PRM would suffice
void CGameServer::EnumSavedGames(CArray<CString>& Out, const CString& Profile) const
{
	Out.Clear();

	CString Path = "AppData:Profiles/";
	if (Profile.IsEmpty())
	{
		if (CurrProfile.IsEmpty()) return;
		Path += CurrProfile;
	}
	else Path += Profile;
	Path += "/Saves";

	IO::CFSBrowser Browser;
	Browser.SetAbsolutePath(Path);
	if (!Browser.IsCurrDirEmpty()) do
	{
		if (Browser.IsCurrEntryFile()) Out.Add(Browser.GetCurrEntryName());
	}
	while (Browser.NextCurrDirEntry());
}
//---------------------------------------------------------------------

bool CGameServer::SavedGameExists(const CString& Name, const CString& Profile)
{
	if (Name.IsEmpty()) FAIL;

	CString Path = "AppData:Profiles/";
	if (Profile.IsEmpty())
	{
		if (CurrProfile.IsEmpty()) FAIL;
		Path += CurrProfile;
	}
	else Path += Profile;
	Path += "/Saves/";
	Path += Name;

	return IOSrv->DirectoryExists(Path);
}
//---------------------------------------------------------------------

bool CGameServer::StartNewGame(const CString& FileName)
{
	n_assert(CurrProfile.IsValid() && !Levels.GetCount() && !Attrs.GetCount());
	CString ContinueDir = "AppData:Profiles/" + CurrProfile + "/Continue";
	if (IOSrv->DirectoryExists(ContinueDir)) { n_verify_dbg(IOSrv->DeleteDirectory(ContinueDir)); }
	return ContinueGame(FileName);
}
//---------------------------------------------------------------------

bool CGameServer::ContinueGame(const CString& FileName)
{
	n_assert(CurrProfile.IsValid() && !Levels.GetCount() && !Attrs.GetCount());

	Data::PParams InitialCommon = DataSrv->LoadPRM(FileName);
	if (!InitialCommon.IsValid()) FAIL;

	Data::PParams SGCommon = DataSrv->ReloadPRM("AppData:Profiles/" + CurrProfile + "/Continue/Main.prm", false);

	Data::PParams GameDesc;
	if (SGCommon.IsValid())
	{
		GameDesc = n_new(Data::CParams);
		InitialCommon->MergeDiff(*GameDesc, *SGCommon);
	}
	else GameDesc = InitialCommon;

	Data::PParams SubSection;
	if (GameDesc->Get<Data::PParams>(SubSection, CStrID("Game")) && SubSection->GetCount())
		SubSection->ToDataDict(Attrs);

	if (GameDesc->Get<Data::PParams>(SubSection, CStrID("Time")) && SubSection->GetCount())
		TimeSrv->Load(*SubSection);
	else TimeSrv->ResetAll();

	// Allow custom gameplay managers to load their data
	EventSrv->FireEvent(CStrID("OnGameDescLoaded"), GameDesc);

	CStrID ActiveLevelID = GetGlobalAttr<CStrID>(CStrID("ActiveLevel"));
	Data::PDataArray LoadedLevels = GetGlobalAttr<Data::PDataArray>(CStrID("LoadedLevels"), NULL);
	if (!LoadedLevels.IsValid())
	{
		LoadedLevels = n_new(Data::CDataArray);
		SetGlobalAttr(CStrID("LoadedLevels"), LoadedLevels);
	}
	if (!LoadedLevels->Contains(ActiveLevelID)) LoadedLevels->Add(ActiveLevelID);

	for (int i = 0; i < LoadedLevels->GetCount(); ++i)
		n_verify(LoadGameLevel(LoadedLevels->Get<CStrID>(i)));

	GameFileName = FileName;

	// Allow custom gameplay managers to load their data
	EventSrv->FireEvent(CStrID("OnGameLoaded"), GameDesc);

	return SetActiveLevel(ActiveLevelID);
}
//---------------------------------------------------------------------

bool CGameServer::LoadGameLevel(CStrID ID)
{
	n_assert(CurrProfile.IsValid());

	CString RelLevelPath = CString(ID.CStr()) + ".prm";

	Data::PParams InitialLvl = DataSrv->LoadPRM("Levels:" + RelLevelPath);
	n_assert(InitialLvl.IsValid());

	CString DiffPath = "AppData:Profiles/" + CurrProfile + "/Continue/Levels/";
	Data::PParams SGLvl = DataSrv->ReloadPRM(DiffPath + RelLevelPath, false);

	Data::PParams LevelDesc;
	if (SGLvl.IsValid())
	{
		LevelDesc = n_new(Data::CParams);
		InitialLvl->MergeDiff(*LevelDesc, *SGLvl);
	}
	else LevelDesc = InitialLvl;

	return LoadLevel(ID, *LevelDesc);
}
//---------------------------------------------------------------------

void CGameServer::UnloadGameLevel(CStrID ID)
{
	int LevelIdx = Levels.FindIndex(ID);
	if (ID == INVALID_INDEX) return;
	n_assert(CurrProfile.IsValid());
	n_verify(CommitLevelDiff(*Levels.ValueAt(LevelIdx)));
	UnloadLevel(ID);
}
//---------------------------------------------------------------------

void CGameServer::PauseGame(bool Pause) const
{
	if (Pause)
	{
		GameTimeSrc->Pause();
		EventSrv->FireEvent(CStrID("OnGamePaused"));
	}
	else
	{
		GameTimeSrc->Unpause();
		EventSrv->FireEvent(CStrID("OnGameUnpaused"));
	}
}
//---------------------------------------------------------------------

bool CGameServer::CommitContinueData()
{
	EntityManager->DeferredDeleteEntities();

	//???!!!here or in Load/Unload level?
	Data::PDataArray LoadedLevels = n_new(Data::CDataArray);
	for (int i = 0; i < Levels.GetCount(); ++i)
		LoadedLevels->Add(Levels.KeyAt(i));
	SetGlobalAttr(CStrID("LoadedLevels"), LoadedLevels);

	Data::PParams GameDesc = DataSrv->LoadPRM(GameFileName);
	if (!GameDesc.IsValid()) FAIL;

	// Save main game file with common data
	Data::PParams SGCommon = n_new(Data::CParams);

	// Save global game attributes diff
	Data::PParams SGGame = n_new(Data::CParams);
	Data::PParams GameSection;
	if (!GameDesc->Get<Data::PParams>(GameSection, CStrID("Game")))
		GameSection = n_new(Data::CParams);
	GameSection->GetDiff(*SGGame, Attrs);
	if (SGGame->GetCount()) SGCommon->Set(CStrID("Game"), SGGame);

	// Time data is never present in the initial game file, so save without diff
	Data::PParams SGTime = n_new(Data::CParams);
	TimeSrv->Save(*SGTime);
	if (SGTime->GetCount()) SGCommon->Set(CStrID("Time"), SGTime);

	// Allow custom gameplay managers to save their data
	EventSrv->FireEvent(CStrID("OnGameSaving"), SGCommon);

	//???diff custom gameplay managers here?
	//mb special Extensions/Plugins section not to affect already saved data by diff

	CString Path = "AppData:Profiles/" + CurrProfile + "/Continue";
	IOSrv->CreateDirectory(Path);
	DataSrv->SavePRM(Path + "/Main.prm", SGCommon);

	//!!!DBG TMP!
	DataSrv->SaveHRD(Path + "/Main.hrd", SGCommon);

	// Save diffs of each level
	for (int i = 0; i < Levels.GetCount(); ++i)
		n_verify(CommitLevelDiff(*Levels.ValueAt(i)));

	OK;
}
//---------------------------------------------------------------------

bool CGameServer::CommitLevelDiff(CGameLevel& Level)
{
	EntityManager->DeferredDeleteEntities();

	Data::PParams SGLevel = n_new(Data::CParams);
	Data::PParams LevelDesc = DataSrv->LoadPRM(CString("Levels:") + Level.GetID().CStr() + ".prm");
	if (!Level.Save(*SGLevel, LevelDesc)) FAIL;
	if (SGLevel->GetCount())
	{
		CString DirName = "AppData:Profiles/" + CurrProfile + "/Continue/Levels/";
		IOSrv->CreateDirectory(DirName);
		DataSrv->SavePRM(DirName + Level.GetID().CStr() + ".prm", SGLevel);

		//!!!DBG TMP!
		DataSrv->SaveHRD(DirName + Level.GetID().CStr() + ".hrd", SGLevel);
	}

	OK;
}
//---------------------------------------------------------------------

bool CGameServer::SaveGame(const CString& Name)
{
	n_assert(CurrProfile.IsValid());

	if (!CommitContinueData()) FAIL;

	//!!!pack savegame! on load can unpack to the override (continue) directory!
	CString ProfileDir = "AppData:Profiles/" + CurrProfile;
	CString SaveDir = ProfileDir + "/Saves/" + Name;
	if (IOSrv->DirectoryExists(SaveDir)) { n_verify_dbg(IOSrv->DeleteDirectory(SaveDir)); }
	IOSrv->CopyDirectory(ProfileDir + "/Continue", SaveDir, true);

	OK;
}
//---------------------------------------------------------------------

bool CGameServer::LoadGame(const CString& Name)
{
	//???event?

	while (Levels.GetCount()) UnloadLevel(Levels.KeyAt(Levels.GetCount() - 1));
	Attrs.Clear();

	CString ProfileDir = "AppData:Profiles/" + CurrProfile;
	CString ContinueDir = ProfileDir + "/Continue";
	if (IOSrv->DirectoryExists(ContinueDir)) { n_verify_dbg(IOSrv->DeleteDirectory(ContinueDir)); }
	IOSrv->CopyDirectory(ProfileDir + "/Saves/" + Name, ContinueDir, true);

	return ContinueGame(GameFileName);
}
//---------------------------------------------------------------------

void CGameServer::ExitGame()
{
	if (!IsGameStarted()) return;

	n_verify(CommitContinueData());
	GameFileName = CString::Empty;

	//???event?

	while (Levels.GetCount()) UnloadLevel(Levels.KeyAt(Levels.GetCount() - 1));
	Attrs.Clear();
}
//---------------------------------------------------------------------

}