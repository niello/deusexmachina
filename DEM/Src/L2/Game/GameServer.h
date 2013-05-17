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
	void		SetActiveLevel(CStrID ID);
	CGameLevel*	GetActiveLevel() const { return ActiveLevel.GetUnsafe(); }
	bool		StartGame(const nString& FileName);
	bool		SaveGame(const nString& Name);
	bool		LoadGame(const nString& Name);
	//???EnumSavedGames?
	//???Profile->GetSaveGamePath?

	//Global attributes

	//Transition service - to move entities from level to level, including store-unload level 1-load level 2-restore case

	nTime	GetTime() const { return GameTimeSrc->GetTime(); }
	nTime	GetFrameTime() const { return GameTimeSrc->GetFrameTime(); }
	bool	IsGamePaused() const { return GameTimeSrc->IsPaused(); }
	void	PauseGame(bool Pause = true) const;
	void	ToggleGamePause() const { PauseGame(!IsGamePaused()); }
};

}

#endif

