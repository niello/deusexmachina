#include "AppStateEditor.h"

#include <App/EditorTool.h>
#include <Events/EventManager.h>
#include <Time/TimeServer.h>
#include <Game/GameServer.h>
#include <Audio/AudioServer.h>
#include <Gfx/GfxServer.h>
#include <AI/AIServer.h>
#include <Physics/PhysicsServer.h>
#include <Input/InputServer.h>
#include <particle/nparticleserver.h>
#include <particle/nparticleserver2.h>
#include <scene/nsceneserver.h>

#include <App/CIDEApp.h> //!!!now only for click cb!

namespace App
{
ImplementRTTI(App::CAppStateEditor, App::CStateHandler);

CAppStateEditor::CAppStateEditor(CStrID StateID):
	CStateHandler(StateID),
    RenderDbgAI(false),
    RenderDbgPhysics(false),
    RenderDbgGfx(false),
    RenderDbgEntities(false),
	pActiveTool(NULL)
{
}
//---------------------------------------------------------------------

void CAppStateEditor::OnStateEnter(CStrID PrevState, PParams Params)
{
	TimeSrv->Update();
	GameSrv->PauseGame();
	InputSrv->EnableContext(CStrID("Editor"));

	SetTool("Select");

	SUBSCRIBE_PEVENT(ToggleGamePause, CAppStateEditor, OnToggleGamePause);
	SUBSCRIBE_PEVENT(ToggleRenderDbgAI, CAppStateEditor, OnToggleRenderDbgAI);
	SUBSCRIBE_PEVENT(ToggleRenderDbgPhysics, CAppStateEditor, OnToggleRenderDbgPhysics);
	SUBSCRIBE_PEVENT(ToggleRenderDbgGfx, CAppStateEditor, OnToggleRenderDbgGfx);
	SUBSCRIBE_PEVENT(ToggleRenderDbgScene, CAppStateEditor, OnToggleRenderDbgScene);
	SUBSCRIBE_PEVENT(ToggleRenderDbgEntities, CAppStateEditor, OnToggleRenderDbgEntities);
}
//---------------------------------------------------------------------

void CAppStateEditor::OnStateLeave(CStrID NextState)
{
	UNSUBSCRIBE_EVENT(ToggleGamePause);
	UNSUBSCRIBE_EVENT(ToggleRenderDbgAI);
	UNSUBSCRIBE_EVENT(ToggleRenderDbgPhysics);
	UNSUBSCRIBE_EVENT(ToggleRenderDbgGfx);
	UNSUBSCRIBE_EVENT(ToggleRenderDbgScene);
	UNSUBSCRIBE_EVENT(ToggleRenderDbgEntities);

	SetTool(NULL);

	InputSrv->DisableContext(CStrID("Editor"));
}
//---------------------------------------------------------------------

CStrID CAppStateEditor::OnFrame()
{
	TimeSrv->Update();
	GfxSrv->Trigger();
	EventMgr->ProcessPendingEvents();

	AudioSrv->BeginScene();
	GameSrv->OnFrame();
	AudioSrv->EndScene();

	nParticleServer::Instance()->Trigger();
	nParticleServer2::Instance()->SetTime(TimeSrv->GetTime());
	nParticleServer2::Instance()->Trigger();

	if (GfxSrv->BeginRender())
	{
		GfxSrv->Render();
		//UISys->Render();
		if (RenderDbgGfx) GfxSrv->RenderDebug();
		if (RenderDbgPhysics) PhysicsSrv->RenderDebug();
		if (RenderDbgEntities) GameSrv->RenderDebug();
		if (RenderDbgAI) AISrv->RenderDebug();
		if (pActiveTool) pActiveTool->Render();
		GfxSrv->EndRender();
	}

	//!!!to some method of memory/core server!
	nMemoryStats memStats = n_dbgmemgetstats();
	CoreSrv->SetGlobal<int>("MemHighWaterSize", memStats.highWaterSize);
	CoreSrv->SetGlobal<int>("MemTotalSize", memStats.totalSize);
	CoreSrv->SetGlobal<int>("MemTotalCount", memStats.totalCount);

	// Editor window receives window messages outside the frame, so clear input here, not in the beginning
	InputSrv->Trigger();

	return ID;
}
//---------------------------------------------------------------------

bool CAppStateEditor::SetTool(LPCSTR Name)
{
	IEditorTool* pTool;

	//!!!USE RTTI!
	if (!Name) pTool = NULL;
	else if (!strcmp(Name, "Select")) pTool = &ToolSelect;
	else if (!strcmp(Name, "Transform")) pTool = &ToolTransform;
	else pTool = NULL;

	if (pActiveTool) pActiveTool->Deactivate();
	pActiveTool = pTool;
	if (pActiveTool) pActiveTool->Activate();
	OK;
}
//---------------------------------------------------------------------

bool CAppStateEditor::OnToggleGamePause(const Events::CEventBase& Event)
{
	GameSrv->ToggleGamePause();
	OK;
}
//---------------------------------------------------------------------

bool CAppStateEditor::OnToggleRenderDbgAI(const Events::CEventBase& Event)
{
	RenderDbgAI = !RenderDbgAI;
	OK;
}
//---------------------------------------------------------------------

bool CAppStateEditor::OnToggleRenderDbgPhysics(const Events::CEventBase& Event)
{
	RenderDbgPhysics = !RenderDbgPhysics;
	OK;
}
//---------------------------------------------------------------------

bool CAppStateEditor::OnToggleRenderDbgGfx(const Events::CEventBase& Event)
{
	RenderDbgGfx = !RenderDbgGfx;
	OK;
}
//---------------------------------------------------------------------

bool CAppStateEditor::OnToggleRenderDbgScene(const Events::CEventBase& Event)
{
	nSceneServer::Instance()->SetRenderDebug(!nSceneServer::Instance()->GetRenderDebug());
	OK;
}
//---------------------------------------------------------------------

bool CAppStateEditor::OnToggleRenderDbgEntities(const Events::CEventBase& Event)
{
	RenderDbgEntities = !RenderDbgEntities;
	OK;
}
//---------------------------------------------------------------------

} // namespace Application
