#pragma once
#ifndef __DEM_L2_GAME_SERVER_H__
#define __DEM_L2_GAME_SERVER_H__

#include <Core/RefCounted.h>
#include <Time/TimeSource.h>
#include <kernel/nprofiler.h>

// The game server setups and runs the game world, consisting of a number
// of active "game entities". Functionality and queries on the game world
// are divided amongst several Manager objects, which are created as
// Singletons during the game server's Open() method. This keeps the
// game server's interface small and clean, and lets Mangalore applications
// easily extend functionality by implementing new, or deriving from
// existing managers.
//
// Based on mangalore Game::CGameServer (C) 2003 RadonLabs GmbH

namespace Game
{
class Entity;
class CManager;

#define GameSrv Game::CGameServer::Instance()

class CGameServer: public Core::CRefCounted
{
	DeclareRTTI;
	DeclareFactory(CGameServer);

protected:

	static CGameServer* Singleton;

	//???to char flags?
	bool					IsOpen;
	bool					IsStarted;
	Time::PTimeSource		GameTimeSrc;
	nArray<Ptr<CManager>>	Managers;

	PROFILER_DECLARE(profGameServerFrame);

public:

	CGameServer();
	virtual ~CGameServer();

	static CGameServer* Instance() { n_assert(Singleton); return Singleton; }

	bool	Open();
	bool	Start();
	void	Stop();
	void	Close();
	void	OnFrame();
	void	RenderDebug();

	void	AttachManager(CManager* pMgr);
	void	RemoveManager(CManager* pMgr);
	void	RemoveAllManagers();

	bool	HasStarted() const { return IsStarted; }
	//void	SetTime(nTime NewTime) { Time = NewTime; }
	nTime	GetTime() const { return GameTimeSrc->GetTime(); } // return Time; }
	//void	SetFrameTime(nTime NewFrameTime) { FrameTime = NewFrameTime; }
	nTime	GetFrameTime() const { return GameTimeSrc->GetFrameTime(); } //return FrameTime; }
	void	ToggleGamePause() const { if (GameTimeSrc->IsPaused()) GameTimeSrc->Unpause(); else GameTimeSrc->Pause(); }
	bool	IsGamePaused() const { return GameTimeSrc->IsPaused(); }
	void	PauseGame(bool Pause = true) const { if (Pause) GameTimeSrc->Pause(); else GameTimeSrc->Unpause(); }
};

RegisterFactory(CGameServer);

}

#endif

