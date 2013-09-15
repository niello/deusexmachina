#pragma once
#ifndef __DEM_L2_GAME_SERVER_H__
#define __DEM_L2_GAME_SERVER_H__

#include <Time/TimeSource.h>
#include <Game/GameLevel.h>
#include <Game/EntityLoader.h>
#include <Game/EntityManager.h>
#include <Game/StaticEnvManager.h>

// Central game engine object. It drives level loading, updating, game saving and loading, entities
// and the main game timer. The server uses events to trigger entities and custom gameplay systems
// from L2 & L3 (like dialogue, quest and item managers).

//!!!!!!on exit game, if profile is set, write SGCommon to continue data, now only levels are in continue!

namespace Game
{
#define GameSrv Game::CGameServer::Instance()

class CGameServer: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CGameServer);

protected:

	bool							IsOpen;
	CString							GameFileName;
	CString							CurrProfile;
	CDataDict						Attrs;

	Time::PTimeSource				GameTimeSrc;
	PEntityManager					EntityManager;
	PStaticEnvManager				StaticEnvManager;

	CDict<CStrID, PGameLevel>		Levels;
	PGameLevel						ActiveLevel;

	CDict<CStrID, PEntityLoader>	Loaders;
	PEntityLoader					DefaultLoader;

	CStrID							EntityUnderMouse;
	bool							HasMouseIsect;
	vector3							MousePos3D;

	bool ValidateLevel(CGameLevel& Level);
	void UpdateMouseIntersectionInfo();
	bool CommitContinueData();
	bool CommitLevelDiff(CGameLevel& Level);

public:

	CGameServer(): IsOpen(false) { __ConstructSingleton; }
	~CGameServer() { n_assert(!IsOpen); __DestructSingleton; }

	bool			Open();
	void			Close();
	void			Trigger();

	void			SetEntityLoader(CStrID Group, PEntityLoader Loader);
	void			ClearEntityLoader(CStrID Group);

	bool			LoadLevel(CStrID ID, const Data::CParams& Desc);
	void			UnloadLevel(CStrID ID);
	bool			SetActiveLevel(CStrID ID);
	CGameLevel*		GetActiveLevel() const { return ActiveLevel.GetUnsafe(); }
	CGameLevel*		GetLevel(CStrID ID) const;
	bool			IsLevelLoaded(CStrID ID) const { return Levels.FindIndex(ID) != INVALID_INDEX; }
	bool			ValidateLevel(CStrID ID);
	bool			ValidateActiveLevel() { return !ActiveLevel.IsValid() || ValidateLevel(*ActiveLevel); }
	bool			ValidateAllLevels();

	void			EnumProfiles(CArray<CString>& Out) const;
	bool			CreateProfile(const CString& Name) const;
	bool			DeleteProfile(const CString& Name) const;
	bool			SetCurrentProfile(const CString& Name);
	const CString&	GetCurrentProfile() const { return CurrProfile; }
	void			EnumSavedGames(CArray<CString>& Out, const CString& Profile = CString::Empty) const;
	bool			SavedGameExists(const CString& Name, const CString& Profile = CString::Empty);

	bool			StartNewGame(const CString& FileName);
	bool			ContinueGame(const CString& FileName);
	bool			SaveGame(const CString& Name);
	bool			LoadGame(const CString& Name);
	bool			LoadGameLevel(CStrID ID);
	void			UnloadGameLevel(CStrID ID);
	void			UnloadAllGameLevels() { while (Levels.GetCount()) UnloadGameLevel(Levels.KeyAt(Levels.GetCount() - 1)); }
	void			ExitGame();
	bool			IsGameStarted() const { return GameFileName.IsValid(); }

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

	CTime			GetTime() const { return GameTimeSrc->GetTime(); }
	CTime			GetFrameTime() const { return GameTimeSrc->GetFrameTime(); }
	DWORD			GetFrameID() const { return GameTimeSrc->GetFrameID(); }
	bool			IsGamePaused() const { return GameTimeSrc->IsPaused(); }
	void			PauseGame(bool Pause = true) const;
	void			ToggleGamePause() const { PauseGame(!IsGamePaused()); }

	bool			HasMouseIntersection() const { return HasMouseIsect; }
	CEntity*		GetEntityUnderMouse() const { return EntityUnderMouse.IsValid() ? EntityManager->GetEntity(EntityUnderMouse) : NULL; }
	CStrID			GetEntityUnderMouseUID() const { return EntityUnderMouse; }
	const vector3&	GetMousePos3D() const { return MousePos3D; }
};

inline CGameLevel* CGameServer::GetLevel(CStrID ID) const
{
	int Idx = Levels.FindIndex(ID);
	return (Idx == INVALID_INDEX) ? NULL : Levels.ValueAt(Idx);
}
//---------------------------------------------------------------------

inline bool CGameServer::ValidateLevel(CStrID ID)
{
	CGameLevel* pLevel = GetLevel(ID);
	return pLevel && ValidateLevel(*pLevel);
}
//---------------------------------------------------------------------

inline bool CGameServer::ValidateAllLevels()
{
	for (int i = 0; i < Levels.GetCount(); ++i)
		if (!ValidateLevel(*Levels.ValueAt(i))) FAIL;
	OK;
}
//---------------------------------------------------------------------

template<class T>
inline void CGameServer::SetGlobalAttr(CStrID ID, const T& Value)
{
	int Idx = Attrs.FindIndex(ID);
	if (Idx == INVALID_INDEX) Attrs.Add(ID, Value);
	else Attrs.ValueAt(Idx).SetTypeValue(Value);
}
//---------------------------------------------------------------------

template<> void CGameServer::SetGlobalAttr(CStrID ID, const Data::CData& Value)
{
	if (Value.IsValid()) Attrs.Set(ID, Value);
	else
	{
		int Idx = Attrs.FindIndex(ID);
		if (Idx != INVALID_INDEX) Attrs.Remove(ID);
	}
}
//---------------------------------------------------------------------

template<class T>
inline const T& CGameServer::GetGlobalAttr(CStrID ID, const T& Default) const
{
	int Idx = Attrs.FindIndex(ID);
	if (Idx == INVALID_INDEX) return Default;
	return Attrs.ValueAt(Idx).GetValue<T>();
}
//---------------------------------------------------------------------

//???ref of ptr? to avoid copying big data
template<class T>
inline bool CGameServer::GetGlobalAttr(T& Out, CStrID ID) const
{
	int Idx = Attrs.FindIndex(ID);
	if (Idx == INVALID_INDEX) FAIL;
	return Attrs.ValueAt(Idx).GetValue<T>(Out);
}
//---------------------------------------------------------------------

//???ref of ptr? to avoid copying big data
template<>
inline bool CGameServer::GetGlobalAttr(Data::CData& Out, CStrID ID) const
{
	int Idx = Attrs.FindIndex(ID);
	if (Idx == INVALID_INDEX) FAIL;
	Out = Attrs.ValueAt(Idx);
	OK;
}
//---------------------------------------------------------------------

}

#endif

