#pragma once
#ifndef __DEM_L2_GAME_SERVER_H__
#define __DEM_L2_GAME_SERVER_H__

#include <Time/TimeSource.h>
#include <Game/GameLevel.h>
#include <Game/EntityLoader.h>
#include <Game/EntityManager.h>
#include <Game/StaticEnvManager.h>
#include <Game/CameraManager.h>

// Central game engine object. It drives level loading, updating, game saving and loading, entities
// and the main game timer. The server uses events to trigger entities and custom gameplay systems
// from L2 & L3 (like dialogue, quest and item managers).

namespace Game
{
#define GameSrv Game::CGameServer::Instance()

class CGameServer: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CGameServer);

protected:

	bool								IsOpen;
	nDictionary<CStrID, Data::CData>	Attrs;

	Time::PTimeSource					GameTimeSrc;
	PEntityManager						EntityManager;
	PStaticEnvManager					StaticEnvManager;
	PCameraManager						CameraManager;

	//!!!selected entities! or per-level?

	nDictionary<CStrID, PGameLevel>		Levels;
	PGameLevel							ActiveLevel;

	nDictionary<CStrID, PEntityLoader>	Loaders;
	PEntityLoader						DefaultLoader;

	CStrID								EntityUnderMouse;
	bool								HasMouseIsect;
	vector3								MousePos3D;

	void UpdateMouseIntersectionInfo();

public:

	CGameServer();
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
	bool			StartGame(const nString& FileName);
	bool			SaveGame(const nString& Name);
	bool			LoadGame(const nString& Name);
	//???EnumSavedGames?
	//???Profile->GetSaveGamePath?

	template<class T>
	void			SetGlobalAttr(CStrID ID, const T& Value);
	template<>
	void			SetGlobalAttr(CStrID ID, const Data::CData& Value);
	template<class T>
	const T&		GetGlobalAttr(CStrID ID) const { return Attrs[ID].GetValue<T>(); }
	template<class T>
	bool			GetGlobalAttr(CStrID ID, T& Out) const;
	template<>
	bool			GetGlobalAttr(CStrID ID, Data::CData& Out) const;
	bool			HasGlobalAttr(CStrID ID) const { return Attrs.FindIndex(ID) != INVALID_INDEX; }

	//Transition service - to move entities from level to level, including store-unload level 1-load level 2-restore case

	nTime			GetTime() const { return GameTimeSrc->GetTime(); }
	nTime			GetFrameTime() const { return GameTimeSrc->GetFrameTime(); }
	bool			IsGamePaused() const { return GameTimeSrc->IsPaused(); }
	void			PauseGame(bool Pause = true) const;
	void			ToggleGamePause() const { PauseGame(!IsGamePaused()); }

	bool			HasMouseIntersection() const { return HasMouseIsect; }
	CEntity*		GetEntityUnderMouse() const { return EntityManager->GetEntity(EntityUnderMouse); }
	CStrID			GetEntityUnderMouseUID() const { return EntityUnderMouse; }
	const vector3&	GetMousePos3D() const { return MousePos3D; }
};

template<class T>
inline void CGameServer::SetGlobalAttr(CStrID ID, const T& Value)
{
	int Idx = Attrs.FindIndex(ID);
	if (Idx == INVALID_INDEX) Attrs.Add(ID, Value);
	else Attrs.ValueAtIndex(Idx).SetTypeValue(Value);
}
//---------------------------------------------------------------------

template<> void CGameServer::SetGlobalAttr(CStrID ID, const Data::CData& Value)
{
	if (Value.IsValid()) Attrs.Set(ID, Value);
	else
	{
		int Idx = Attrs.FindIndex(ID);
		if (Idx != INVALID_INDEX) Attrs.Erase(ID);
	}
}
//---------------------------------------------------------------------

//???ref of ptr? to avoid copying big data
template<class T>
inline bool CGameServer::GetGlobalAttr(CStrID ID, T& Out) const
{
	int Idx = Attrs.FindIndex(ID);
	if (Idx == INVALID_INDEX) FAIL;
	return Attrs.ValueAtIndex(Idx).GetValue<T>(Out);
}
//---------------------------------------------------------------------

//???ref of ptr? to avoid copying big data
template<>
inline bool CGameServer::GetGlobalAttr(CStrID ID, Data::CData& Out) const
{
	int Idx = Attrs.FindIndex(ID);
	if (Idx == INVALID_INDEX) FAIL;
	Out = Attrs.ValueAtIndex(Idx);
	OK;
}
//---------------------------------------------------------------------

}

#endif

