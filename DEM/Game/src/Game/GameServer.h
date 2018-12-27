#pragma once
#ifndef __DEM_L2_GAME_SERVER_H__
#define __DEM_L2_GAME_SERVER_H__

#include <Data/Singleton.h>
#include <Core/TimeSource.h>
#include <Game/EntityManager.h>
#include <Data/HandleManager.h>
#include <Data/Data.h>

// Central game engine object. It drives level loading, updating, game saving and loading, entities
// and the main game timer. The server uses events to trigger entities and custom gameplay systems
// from L2 & L3 (like dialogue, quest and item managers).

//!!!!!!on exit game, if profile is set, write SGCommon to continue data, now only levels are in continue!

namespace Data
{
	class CParams;
}

namespace Render
{
	class CGPUDriver;	// For level validation only //???redesign?
}

namespace Scene
{
	class CSceneNode;
}

namespace Game
{
typedef Ptr<class CGameLevel> PGameLevel;
class CGameLevelView;

#define GameSrv Game::CGameServer::Instance()

class CGameServer
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CGameServer);

protected:

	bool							IsOpen;
	CString							GameFileName;
	CString							CurrProfile;
	CDataDict						Attrs;

	Core::PTimeSource				GameTimeSrc;
	CEntityManager					EntityManager;
	//CCameraManager				CameraManager;

	CDict<CStrID, PGameLevel>		Levels;
	CArray<CGameLevelView*>			LevelViews;
	Data::CHandleManager			LevelViewHandles;
	CArray<Scene::CSceneNode*>		DefferedNodes;	// See Trigger() method, cached to avoid per-frame reallocations

	bool CommitContinueData();
	bool CommitLevelDiff(CGameLevel& Level);

public:

	CGameServer(): IsOpen(false), LevelViews(0, 1), LevelViewHandles(1) { __ConstructSingleton; }
	~CGameServer() { n_assert(!IsOpen); __DestructSingleton; }

	bool			Open();
	void			Close();
	void			Trigger();

	bool			LoadLevel(CStrID ID, const Data::CParams& Desc);
	void			UnloadLevel(CStrID ID);
	CGameLevel*		GetLevel(CStrID ID) const;
	bool			IsLevelLoaded(CStrID ID) const { return Levels.FindIndex(ID) != INVALID_INDEX; }
	bool			ValidateAllLevels(Render::CGPUDriver* pGPU);

	HHandle			CreateLevelView(CStrID LevelID);
	void			DestroyLevelView(HHandle hView);
	CGameLevelView*	GetLevelView(HHandle hView) { return (CGameLevelView*)LevelViewHandles.GetHandleData(hView); }

	void			EnumProfiles(CArray<CString>& Out) const;
	bool			CreateProfile(const char* pName) const;
	bool			DeleteProfile(const char* pName) const;
	bool			SetCurrentProfile(const char* pName);
	const CString&	GetCurrentProfile() const { return CurrProfile; }
	void			EnumSavedGames(CArray<CString>& Out, const char* pProfile = NULL) const;
	bool			SavedGameExists(const char* pName, const char* pProfile = NULL);

	bool			StartNewGame(const char* pFileName);
	bool			ContinueGame(const char* pFileName);
	bool			SaveGame(const char* pName);
	bool			LoadGame(const char* pName);
	bool			LoadGameLevel(CStrID ID);
	void			UnloadGameLevel(CStrID ID);
	void			UnloadAllGameLevels() { while (Levels.GetCount()) UnloadGameLevel(Levels.KeyAt(Levels.GetCount() - 1)); }
	void			ExitGame();
	bool			IsGameStarted() const { return GameFileName.IsValid(); }

	//???need or use core server constants?
	template<class T>
	void			SetGlobalAttr(CStrID ID, const T& Value);
	template<>
	void			SetGlobalAttr(CStrID ID, const Data::CData& Value);
	template<class T>
	const T&		GetGlobalAttr(CStrID ID) const { return Attrs[ID].GetValue<T>(); }
	template<class T>
	const T&		GetGlobalAttr(CStrID ID, const T& Default) const;
	template<class T>
	bool			GetGlobalAttr(T& Out, CStrID ID) const;
	template<>
	bool			GetGlobalAttr(Data::CData& Out, CStrID ID) const;
	bool			HasGlobalAttr(CStrID ID) const { return Attrs.FindIndex(ID) != INVALID_INDEX; }

	CEntityManager*	GetEntityMgr() { return &EntityManager; }
	CTime			GetTime() const { return GameTimeSrc->GetTime(); }
	CTime			GetFrameTime() const { return GameTimeSrc->GetFrameTime(); }
	UPTR			GetFrameID() const { return GameTimeSrc->GetFrameID(); }
	bool			IsGamePaused() const { return GameTimeSrc->IsPaused(); }
	void			PauseGame(bool Pause = true) const;
	void			ToggleGamePause() const { PauseGame(!IsGamePaused()); }
};

inline CGameLevel* CGameServer::GetLevel(CStrID ID) const
{
	IPTR Idx = Levels.FindIndex(ID);
	return (Idx == INVALID_INDEX) ? NULL : Levels.ValueAt(Idx);
}
//---------------------------------------------------------------------

template<class T>
inline void CGameServer::SetGlobalAttr(CStrID ID, const T& Value)
{
	IPTR Idx = Attrs.FindIndex(ID);
	if (Idx == INVALID_INDEX) Attrs.Add(ID, Value);
	else Attrs.ValueAt(Idx).SetTypeValue(Value);
}
//---------------------------------------------------------------------

template<> void CGameServer::SetGlobalAttr(CStrID ID, const Data::CData& Value)
{
	if (Value.IsValid()) Attrs.Set(ID, Value);
	else
	{
		IPTR Idx = Attrs.FindIndex(ID);
		if (Idx != INVALID_INDEX) Attrs.Remove(ID);
	}
}
//---------------------------------------------------------------------

template<class T>
inline const T& CGameServer::GetGlobalAttr(CStrID ID, const T& Default) const
{
	IPTR Idx = Attrs.FindIndex(ID);
	if (Idx == INVALID_INDEX) return Default;
	return Attrs.ValueAt(Idx).GetValue<T>();
}
//---------------------------------------------------------------------

//???ref of ptr? to avoid copying big data
template<class T>
inline bool CGameServer::GetGlobalAttr(T& Out, CStrID ID) const
{
	IPTR Idx = Attrs.FindIndex(ID);
	if (Idx == INVALID_INDEX) FAIL;
	return Attrs.ValueAt(Idx).GetValue<T>(Out);
}
//---------------------------------------------------------------------

//???ref of ptr? to avoid copying big data
template<>
inline bool CGameServer::GetGlobalAttr(Data::CData& Out, CStrID ID) const
{
	IPTR Idx = Attrs.FindIndex(ID);
	if (Idx == INVALID_INDEX) FAIL;
	Out = Attrs.ValueAt(Idx);
	OK;
}
//---------------------------------------------------------------------

}

#endif

