#pragma once
#ifndef __DEM_L2_GAME_SERVER_H__
#define __DEM_L2_GAME_SERVER_H__

#include <Time/TimeSource.h>
#include <Game/GameLevel.h>
#include <Game/EntityManager.h>
#include <Game/EntityLoader.h>

// Central game engine object. It drives level loading, updating, game saving and loading, entities
// and the main game timer. The server uses events to trigger entities and custom gameplay systems
// from L2 & L3 (like dialogue, quest and item managers).

namespace Game
{
class Entity;

#define GameSrv Game::CGameServer::Instance()

class CGameServer: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CGameServer);

protected:

	bool								IsOpen;
	Time::PTimeSource					GameTimeSrc;
	PEntityManager						EntityManager;
	nDictionary<CStrID, PGameLevel>		Levels;
	PGameLevel							ActiveLevel;
	nDictionary<CStrID, Data::CData>	Attrs;

	PEntityLoader						DefaultLoader;
	nDictionary<CStrID, PEntityLoader>	Loaders;

public:

	CGameServer();
	~CGameServer() { n_assert(!IsOpen); __DestructSingleton; }

	bool		Open();
	void		Close();
	void		Trigger();

	//!!!can get entity under mouse here! ActiveLevel->GetEntityAtScreenPos(mouse.x, mouse.y)
	//SEE commented OnFrame method in the cpp
	//
	//CStrID	EntityUnderMouse; - if already deleted, repeat request?
	//vector3	MousePos3D;
	//vector3	UpVector;
	//bool	MouseIntersection;
	//EntityUnderMouse(CStrID::Empty),
	//MouseIntersection(false)
	//bool			HasMouseIntersection() const { return MouseIntersection; }
	//CEntity*		GetEntityUnderMouse() const; //???write 2 versions, physics-based and mesh-based?
	//const vector3&	GetMousePos3D() const { return MousePos3D; }
	//const vector3&	GetUpVector() const { return UpVector; }

	//!!!remove and use GetActiveLevel!
	void		RenderCurrentLevel() { if (ActiveLevel.IsValid()) ActiveLevel->RenderScene(); }
	void		RenderCurrentLevelDebug() { if (ActiveLevel.IsValid()) ActiveLevel->RenderDebug(); }

	void		SetEntityLoader(CStrID Group, PEntityLoader Loader);
	void		ClearEntityLoader(CStrID Group);

	bool		LoadLevel(CStrID ID, const Data::CParams& Desc);
	void		UnloadLevel(CStrID ID);
	bool		SetActiveLevel(CStrID ID);
	CGameLevel*	GetActiveLevel() const { return ActiveLevel.GetUnsafe(); }
	bool		StartGame(const nString& FileName);
	bool		SaveGame(const nString& Name);
	bool		LoadGame(const nString& Name);
	//???EnumSavedGames?
	//???Profile->GetSaveGamePath?

	template<class T>
	void		SetGlobalAttr(CStrID ID, const T& Value);
	template<>
	void		SetGlobalAttr(CStrID ID, const Data::CData& Value);
	template<class T>
	const T&	GetGlobalAttr(CStrID ID) const { return Attrs[ID].GetValue<T>(); }
	template<class T>
	bool		GetGlobalAttr(CStrID ID, T& Out) const;
	template<>
	bool		GetGlobalAttr(CStrID ID, Data::CData& Out) const;
	bool		HasGlobalAttr(CStrID ID) const { return Attrs.FindIndex(ID) != INVALID_INDEX; }

	//Transition service - to move entities from level to level, including store-unload level 1-load level 2-restore case

	nTime		GetTime() const { return GameTimeSrc->GetTime(); }
	nTime		GetFrameTime() const { return GameTimeSrc->GetFrameTime(); }
	bool		IsGamePaused() const { return GameTimeSrc->IsPaused(); }
	void		PauseGame(bool Pause = true) const;
	void		ToggleGamePause() const { PauseGame(!IsGamePaused()); }
};

inline bool CGameServer::SetActiveLevel(CStrID ID)
{
	if (ID.IsValid())
	{
		int LevelIdx = Levels.FindIndex(ID);
		if (LevelIdx == INVALID_INDEX) FAIL;
		ActiveLevel = Levels.ValueAtIndex(LevelIdx);
	}
	else ActiveLevel = NULL;
	OK;
}
//---------------------------------------------------------------------

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

