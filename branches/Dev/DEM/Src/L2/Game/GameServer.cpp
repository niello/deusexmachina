#include "GameServer.h"

#include <Game/Manager.h>
#include <Data/DataServer.h>
#include <Time/TimeServer.h>
#include <Events/EventManager.h>

namespace Game
{
ImplementRTTI(Game::CGameServer, Core::CRefCounted);
ImplementFactory(Game::CGameServer);

CGameServer* CGameServer::Singleton = NULL;

CGameServer::CGameServer(): IsOpen(false), IsStarted(false)
{
	n_assert(!Singleton);
	Singleton = this;

	DataSrv->SetAssign("iao", "game:iao");

	GameTimeSrc = Time::CTimeSource::Create();

	PROFILER_INIT(profGameServerFrame, "profMangaGameServerFrame");
}
//---------------------------------------------------------------------

CGameServer::~CGameServer()
{
	n_assert(!IsOpen);

	GameTimeSrc = NULL;

	n_assert(Singleton);
	Singleton = NULL;
}
//---------------------------------------------------------------------

bool CGameServer::Open()
{
    n_assert(!IsOpen && !IsStarted);
	TimeSrv->AttachTimeSource(CStrID("Game"), GameTimeSrc);
    IsOpen = true;
    return true;
}
//---------------------------------------------------------------------

// Start the game world, called after loading has completed.
bool CGameServer::Start()
{
	n_assert(IsOpen);
	n_assert(!IsStarted);

	// Call the OnStart() method on all Entities and Managers //???is entities-managers order important?
	EventMgr->FireEvent(CStrID("OnStart"));

	IsStarted = true;
	return true;
}
//---------------------------------------------------------------------

// Stop the game world, called before the world(current level) is cleaned up.
void CGameServer::Stop()
{
	n_assert(IsOpen && IsStarted);
	IsStarted = false;
	GameTimeSrc->Reset();
}
//---------------------------------------------------------------------

void CGameServer::Close()
{
	n_assert(!IsStarted && IsOpen);

	TimeSrv->RemoveTimeSource(CStrID("Game"));

	for (int i = 0; i < Managers.Size(); i++) Managers[i]->Deactivate();
	Managers.Clear();

	IsOpen = false;
}
//---------------------------------------------------------------------

void CGameServer::OnFrame()
{
	PROFILER_START(profGameServerFrame);
	EventMgr->FireEvent(CStrID("OnFrame"));
	PROFILER_STOP(profGameServerFrame);
}
//---------------------------------------------------------------------

void CGameServer::AttachManager(CManager* pMgr)
{
	n_assert(pMgr);
	pMgr->Activate();
	Managers.Append(pMgr);
}
//---------------------------------------------------------------------

void CGameServer::RemoveManager(CManager* pMgr)
{
	n_assert(pMgr);
	int Idx = Managers.FindIndex(pMgr);
	if (Idx != INVALID_INDEX)
	{
		Managers[Idx]->Deactivate();
		Managers.Erase(Idx);
	}
}
//---------------------------------------------------------------------
	
void CGameServer::RemoveAllManagers()
{
	for (int i = 0; i < Managers.Size(); i++) Managers[i]->Deactivate();
	Managers.Clear();
}
//---------------------------------------------------------------------

void CGameServer::RenderDebug()
{
	//GFX
	/*
	nGfxServer2::Instance()->BeginShapes();
	EventMgr->FireEvent(CStrID("OnRenderDebug"));
	nGfxServer2::Instance()->EndShapes();
	*/
}
//---------------------------------------------------------------------

} // namespace Game
