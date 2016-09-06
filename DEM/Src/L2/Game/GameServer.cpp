#include "GameServer.h"

#include <Game/GameLevelView.h>
#include <Game/SceneNodeValidateAttrs.h>
#include <AI/AIServer.h>
#include <Frame/SceneNodeValidateResources.h>
#include <Frame/SceneNodeUpdateInSPS.h>
#include <Frame/NodeAttrCamera.h>
#include <Physics/PhysicsLevel.h>
#include <IO/IOServer.h>
#include <IO/FSBrowser.h>
#include <UI/UIServer.h>
#include <Time/TimeServer.h>
#include <Scripting/ScriptObject.h>
#include <Data/ParamsUtils.h>
#include <Data/DataArray.h>
#include <Events/EventServer.h>

namespace Game
{
__ImplementClassNoFactory(Game::CGameServer, Core::CObject);
__ImplementSingleton(CGameServer);

bool CGameServer::Open()
{
	n_assert(!IsOpen);

	GameTimeSrc = n_new(Time::CTimeSource);
	TimeSrv->AttachTimeSource(CStrID("Game"), GameTimeSrc);

	IsOpen = true;
	OK;
}
//---------------------------------------------------------------------

void CGameServer::Close()
{
	n_assert(IsOpen);

	for (UPTR i = 0; i < LevelViews.GetCount(); ++i) n_delete(LevelViews[i]);
	LevelViews.Clear();
	LevelViewHandles.Clear();

	TimeSrv->RemoveTimeSource(CStrID("Game"));
	GameTimeSrc = NULL;

	IsOpen = false;
}
//---------------------------------------------------------------------

void CGameServer::Trigger()
{
	EntityMgr->DeferredDeleteEntities();

	EventSrv->FireEvent(CStrID("OnBeginFrame"));

	AISrv->Trigger(); // Pathfinding queries inside

	float FrameTime = (float)GameTimeSrc->GetFrameTime();

	UPTR ViewCount = LevelViews.GetCount();
	vector3* pCOIArray = ViewCount ? (vector3*)_malloca(sizeof(vector3) * ViewCount) : NULL;

	for (UPTR i = 0; i < Levels.GetCount(); ++i)
	{
		CGameLevel* pLevel = Levels.ValueAt(i);

		pLevel->FireEvent(CStrID("BeforeTransforms"));

		UPTR COICount = 0;
		for (UPTR i = 0; i < ViewCount; ++i)
		{
			CGameLevelView* pView = LevelViews[i];
			if (pView->GetLevel() == pLevel)
				pCOIArray[COICount++] = pView->GetCenterOfInterest();
		}

		Scene::CSceneNode* pSceneRoot = pLevel->GetSceneRoot();

		DefferedNodes.Clear(false);
		if (pSceneRoot) pSceneRoot->UpdateTransform(pCOIArray, COICount, false, &DefferedNodes);

		Physics::CPhysicsLevel* pPhysLvl = pLevel->GetPhysics();
		if (pPhysLvl)
		{
			pLevel->FireEvent(CStrID("BeforePhysics"));
			pPhysLvl->Trigger(FrameTime);
			pLevel->FireEvent(CStrID("AfterPhysics"));
		}

		for (UPTR i = 0; i < DefferedNodes.GetCount(); ++i)
			DefferedNodes[i]->UpdateTransform(pCOIArray, COICount, true, NULL);

		pLevel->FireEvent(CStrID("AfterTransforms"));

		if (pSceneRoot && COICount > 0)
		{
			Frame::CSceneNodeUpdateInSPS Visitor;
			Visitor.pSPS = pLevel->GetSPS();
			pSceneRoot->AcceptVisitor(Visitor);
		}
	}

	if (pCOIArray) _freea(pCOIArray);

	EventSrv->FireEvent(CStrID("OnEndFrame"));
}
//---------------------------------------------------------------------

bool CGameServer::LoadLevel(CStrID ID, const Data::CParams& Desc)
{
	//???update already existing objects or unload the level?
	IPTR LevelIdx = Levels.FindIndex(ID);
	if (LevelIdx != INVALID_INDEX) FAIL;

	PGameLevel Level = n_new(CGameLevel);
	if (!Level->Load(ID, Desc)) FAIL;
	Levels.Add(Level->GetID(), Level);

	//!!!to view loading!
	//Data::PDataArray SelArray;
	//if (Desc.Get(SelArray, CStrID("SelectedEntities")))
	//	for (UPTR i = 0; i < SelArray->GetCount(); ++i)
	//		Level->AddToSelection(SelArray->Get<CStrID>(i));

	Data::PParams P = n_new(Data::CParams);
	P->Set(CStrID("ID"), ID);
	EventSrv->FireEvent(CStrID("OnLevelLoaded"), P);

	OK;
}
//---------------------------------------------------------------------

void CGameServer::UnloadLevel(CStrID ID)
{
	IPTR LevelIdx = Levels.FindIndex(ID);
	if (ID == INVALID_INDEX) return;

	PGameLevel Level = Levels.ValueAt(LevelIdx);

	Data::PParams P = n_new(Data::CParams(1));
	P->Set(CStrID("ID"), ID);
	EventSrv->FireEvent(CStrID("OnLevelUnloading"), P);

	UPTR i = 0;
	while (i < LevelViews.GetCount())
	{
		CGameLevelView* pView = LevelViews[i];
		if (pView->GetLevel() == Level)
		{
			LevelViewHandles.CloseHandle(pView->GetHandle());
			LevelViews.RemoveAt(i);
			n_delete(pView);
		}
		else ++i;
	}

	Level->FireEvent(CStrID("OnEntitiesUnloading"));
	EntityMgr->DeleteEntities(*Level);
	StaticEnvMgr->DeleteStaticObjects(*Level);
	Level->FireEvent(CStrID("OnEntitiesUnloaded"));

	Levels.RemoveAt(LevelIdx);

	Level->Term();

	EventSrv->FireEvent(CStrID("OnLevelUnloaded"), P);

	n_assert_dbg(Level->GetRefCount() == 1);
}
//---------------------------------------------------------------------

HHandle CGameServer::CreateLevelView(CStrID LevelID)
{
	IPTR Idx = Levels.FindIndex(LevelID);
	if (Idx == INVALID_INDEX) return INVALID_HANDLE;

	CGameLevel* pLevel = Levels.ValueAt(Idx);
	if (!pLevel) return INVALID_HANDLE;

	CGameLevelView* pView = n_new(CGameLevelView);

	HHandle hView = LevelViewHandles.OpenHandle(pView);
	if (hView == INVALID_HANDLE)
	{
		n_delete(pView);
		return INVALID_HANDLE;
	}

	if (!pView->Setup(*pLevel, hView))
	{
		LevelViewHandles.CloseHandle(hView);
		n_delete(pView);
		return INVALID_HANDLE;
	}

	LevelViews.Add(pView);
	return hView;
}
//---------------------------------------------------------------------

void CGameServer::DestroyLevelView(HHandle hView)
{
	CGameLevelView* pView = (CGameLevelView*)LevelViewHandles.GetHandleData(hView);
	if (!pView) return;
	LevelViewHandles.CloseHandle(hView);
	LevelViews.RemoveByValue(pView);
	n_delete(pView);
}
//---------------------------------------------------------------------

void CGameServer::EnumProfiles(CArray<CString>& Out) const
{
	Out.Clear();

	CString ProfilesDir("AppData:Profiles");
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

bool CGameServer::CreateProfile(const char* pName) const
{
	CString ProfileDir("AppData:Profiles/");
	ProfileDir += pName;
	if (IOSrv->DirectoryExists(ProfileDir)) FAIL;
	return IOSrv->CreateDirectory(ProfileDir + "/Saves") &&
		IOSrv->CreateDirectory(ProfileDir + "/Continue/Levels");
}
//---------------------------------------------------------------------

bool CGameServer::DeleteProfile(const char* pName) const
{
	if (CurrProfile == pName && GameFileName.IsValid()) FAIL; //???or stop game and set current to empty?
	return IOSrv->DeleteDirectory(CString("AppData:Profiles/") + pName);
}
//---------------------------------------------------------------------

bool CGameServer::SetCurrentProfile(const char* pName)
{
	CurrProfile = pName;

	//!!!load and apply settings!

	OK;
}
//---------------------------------------------------------------------

//!!!pack saves to files! single PRM would suffice
void CGameServer::EnumSavedGames(CArray<CString>& Out, const char* pProfile) const
{
	Out.Clear();

	CString Path("AppData:Profiles/");
	if (!pProfile || !*pProfile)
	{
		if (CurrProfile.IsEmpty()) return;
		Path += CurrProfile;
	}
	else Path += pProfile;
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

bool CGameServer::SavedGameExists(const char* pName, const char* pProfile)
{
	if (!pName || !*pName) FAIL;

	CString Path("AppData:Profiles/");
	if (!pProfile || !*pProfile)
	{
		if (CurrProfile.IsEmpty()) FAIL;
		Path += CurrProfile;
	}
	else Path += pProfile;
	Path += "/Saves/";
	Path += pName;

	return IOSrv->DirectoryExists(Path);
}
//---------------------------------------------------------------------

bool CGameServer::StartNewGame(const char* pFileName)
{
	n_assert(CurrProfile.IsValid() && !Levels.GetCount() && !Attrs.GetCount());
	CString ContinueDir = "AppData:Profiles/" + CurrProfile + "/Continue";
	if (IOSrv->DirectoryExists(ContinueDir)) { n_verify_dbg(IOSrv->DeleteDirectory(ContinueDir)); }
	return ContinueGame(pFileName);
}
//---------------------------------------------------------------------

bool CGameServer::ContinueGame(const char* pFileName)
{
	n_assert(CurrProfile.IsValid() && !Levels.GetCount() && !Attrs.GetCount());

	Data::PParams InitialCommon;
	if (!ParamsUtils::LoadParamsFromPRM(pFileName, InitialCommon)) FAIL;
	if (InitialCommon.IsNullPtr()) FAIL;

	Data::PParams SGCommon;
	ParamsUtils::LoadParamsFromPRM("AppData:Profiles/" + CurrProfile + "/Continue/Main.prm", SGCommon);

	Data::PParams GameDesc;
	if (SGCommon.IsValidPtr())
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
	EventSrv->FireEvent(CStrID("OnGameDescLoaded"), GameDesc->Get<Data::PParams>(CStrID("Managers"), NULL));

	Data::PDataArray LoadedLevels = GetGlobalAttr<Data::PDataArray>(CStrID("LoadedLevels"), NULL);
	if (LoadedLevels.IsNullPtr())
	{
		LoadedLevels = n_new(Data::CDataArray);
		SetGlobalAttr(CStrID("LoadedLevels"), LoadedLevels);
	}

	for (UPTR i = 0; i < LoadedLevels->GetCount(); ++i)
		n_verify(LoadGameLevel(LoadedLevels->Get<CStrID>(i)));

	GameFileName = pFileName;

	EventSrv->FireEvent(CStrID("OnGameLoaded"), GameDesc);

	OK;
}
//---------------------------------------------------------------------

bool CGameServer::LoadGameLevel(CStrID ID)
{
	n_assert(CurrProfile.IsValid());

	CString RelLevelPath = CString(ID.CStr()) + ".prm";

	Data::PParams InitialLvl;
	if (!ParamsUtils::LoadParamsFromPRM("Levels:" + RelLevelPath, InitialLvl)) FAIL;
	n_assert(InitialLvl.IsValidPtr());

	CString DiffPath = "AppData:Profiles/" + CurrProfile + "/Continue/Levels/";
	Data::PParams SGLvl;
	ParamsUtils::LoadParamsFromPRM(DiffPath + RelLevelPath, SGLvl);

	Data::PParams LevelDesc;
	if (SGLvl.IsValidPtr())
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
	IPTR LevelIdx = Levels.FindIndex(ID);
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
	EntityManager.DeferredDeleteEntities();

	//???!!!here or in Load/Unload level?
	Data::PDataArray LoadedLevels = n_new(Data::CDataArray);
	for (UPTR i = 0; i < Levels.GetCount(); ++i)
		LoadedLevels->Add(Levels.KeyAt(i));
	SetGlobalAttr(CStrID("LoadedLevels"), LoadedLevels);

	Data::PParams GameDesc;
	if (!ParamsUtils::LoadParamsFromPRM(GameFileName, GameDesc)) FAIL;
	if (GameDesc.IsNullPtr()) FAIL;

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
	Data::PParams SGManagers;
	Data::PParams NewManagers = n_new(Data::CParams);
	EventSrv->FireEvent(CStrID("OnGameSaving"), NewManagers);
	Data::PParams InitialManagers;
	if (GameDesc->Get<Data::PParams>(InitialManagers, CStrID("Managers")))
	{
		SGManagers = n_new(Data::CParams);
		InitialManagers->GetDiff(*SGManagers, *NewManagers);
	}
	else SGManagers = NewManagers;
	if (SGManagers->GetCount()) SGCommon->Set(CStrID("Managers"), SGManagers);

	CString Path = "AppData:Profiles/" + CurrProfile + "/Continue";
	IOSrv->CreateDirectory(Path);
	if (!ParamsUtils::SaveParamsToPRM(Path + "/Main.prm", *SGCommon.GetUnsafe())) FAIL;

	//!!!DBG TMP!
	if (!ParamsUtils::SaveParamsToHRD(Path + "/Main.hrd", *SGCommon.GetUnsafe())) FAIL;

	// Save diffs of each level
	for (UPTR i = 0; i < Levels.GetCount(); ++i)
		n_verify(CommitLevelDiff(*Levels.ValueAt(i)));

	OK;
}
//---------------------------------------------------------------------

bool CGameServer::CommitLevelDiff(CGameLevel& Level)
{
	EntityManager.DeferredDeleteEntities();

	Data::PParams SGLevel = n_new(Data::CParams);
	Data::PParams LevelDesc;
	if (!ParamsUtils::LoadParamsFromPRM(CString("Levels:") + Level.GetID().CStr() + ".prm", LevelDesc)) FAIL;
	if (!Level.Save(*SGLevel, LevelDesc)) FAIL;
	if (SGLevel->GetCount())
	{
		CString DirName = "AppData:Profiles/" + CurrProfile + "/Continue/Levels/";
		IOSrv->CreateDirectory(DirName);
		if (!ParamsUtils::SaveParamsToPRM(DirName + Level.GetID().CStr() + ".prm", *SGLevel.GetUnsafe())) FAIL;

		//!!!DBG TMP!
		if (!ParamsUtils::SaveParamsToHRD(DirName + Level.GetID().CStr() + ".hrd", *SGLevel.GetUnsafe())) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

bool CGameServer::SaveGame(const char* pName)
{
	n_assert(CurrProfile.IsValid());

	if (!CommitContinueData()) FAIL;

	//!!!pack savegame! on load can unpack to the override (continue) directory!
	CString ProfileDir = "AppData:Profiles/" + CurrProfile;
	CString SaveDir = ProfileDir + "/Saves/" + pName;
	if (IOSrv->DirectoryExists(SaveDir)) { n_verify_dbg(IOSrv->DeleteDirectory(SaveDir)); }
	IOSrv->CopyDirectory(ProfileDir + "/Continue", SaveDir, true);

	OK;
}
//---------------------------------------------------------------------

bool CGameServer::LoadGame(const char* pName)
{
	//???event?

	while (Levels.GetCount()) UnloadLevel(Levels.KeyAt(Levels.GetCount() - 1));
	Attrs.Clear();

	CString ProfileDir = "AppData:Profiles/" + CurrProfile;
	CString ContinueDir = ProfileDir + "/Continue";
	if (IOSrv->DirectoryExists(ContinueDir)) { n_verify_dbg(IOSrv->DeleteDirectory(ContinueDir)); }
	IOSrv->CopyDirectory(ProfileDir + "/Saves/" + pName, ContinueDir, true);

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