#include "AppStateEditor.h"

//#include <Story/Quests/QuestSystem.h>
//#include <Story/Dlg/DlgSystem.h>
//#include <UI/UISystem.h>
//#include <UI/IngameScreen.h>
#include <Events/EventManager.h>
#include <Time/TimeServer.h>
#include <Loading/LoaderServer.h> //???unload level here?
#include <Game/GameServer.h>
#include <Audio/AudioServer.h>
#include <Gfx/GfxServer.h>
#include <AI/AIServer.h>
#include <Physics/PhysicsServer.h>
#include <Input/InputServer.h>
#include <Input/Events/MouseBtnDown.h>
#include <Input/Events/MouseBtnUp.h>
#include <particle/nparticleserver.h>
#include <particle/nparticleserver2.h>
#include <scene/nsceneserver.h>

#include <App/CIDEApp.h> //!!!now only for click cb!

namespace App
{
ImplementRTTI(App::CAppStateEditor, App::CStateHandler);
//ImplementFactory(App::CAppStateEditor);

CAppStateEditor::CAppStateEditor(CStrID StateID, PCIDEApp pApp):
	CStateHandler(StateID),
    RenderDbgAI(false),
    RenderDbgPhysics(false),
    RenderDbgGfx(false),
    RenderDbgEntities(false),
	CIDEApp(pApp)
{
	PROFILER_INIT(profCompleteFrame, "profMangaCompleteFrame");
	PROFILER_INIT(profParticleUpdates, "profMangaParticleUpdates");
	PROFILER_INIT(profRender, "profMangaRender");
}
//---------------------------------------------------------------------

CAppStateEditor::~CAppStateEditor()
{
}
//---------------------------------------------------------------------

void CAppStateEditor::OnStateEnter(CStrID PrevState, PParams Params)
{
	// Loaded from db, not reset. TimeMgr now auto-reset timers if can't load them.
	//TimeSrv->ResetAll();
	TimeSrv->Update();

	GameSrv->PauseGame();

	RenderDbgPhysics = false;
	RenderDbgGfx = false;

	//InputSrv->EnableContext(CStrID("Game"));
	InputSrv->EnableContext(CStrID("Editor"));

	SUBSCRIBE_INPUT_EVENT(MouseBtnDown, CAppStateEditor, OnMouseBtnDown, Input::InputPriority_Raw);
	SUBSCRIBE_INPUT_EVENT(MouseBtnUp, CAppStateEditor, OnMouseBtnUp, Input::InputPriority_Raw);
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
	UNSUBSCRIBE_EVENT(MouseBtnDown);
	UNSUBSCRIBE_EVENT(MouseBtnUp);
	UNSUBSCRIBE_EVENT(ToggleGamePause);
	UNSUBSCRIBE_EVENT(ToggleRenderDbgAI);
	UNSUBSCRIBE_EVENT(ToggleRenderDbgPhysics);
	UNSUBSCRIBE_EVENT(ToggleRenderDbgGfx);
	UNSUBSCRIBE_EVENT(ToggleRenderDbgScene);
	UNSUBSCRIBE_EVENT(ToggleRenderDbgEntities);

	//InputSrv->DisableContext(CStrID("Game"));
	InputSrv->DisableContext(CStrID("Editor"));
}
//---------------------------------------------------------------------

CStrID CAppStateEditor::OnFrame()
{
	PROFILER_START(profCompleteFrame);
	bool Running = true;

	TimeSrv->Update();

	GfxSrv->Trigger();

	EventMgr->ProcessPendingEvents();

	//QuestSys->Trigger();
	//DlgSys->Trigger();

	AudioSrv->BeginScene();
	GameSrv->OnFrame();
	AudioSrv->EndScene();

	PROFILER_START(profParticleUpdates);
	nParticleServer::Instance()->Trigger();
	nParticleServer2::Instance()->SetTime(TimeSrv->GetTime());
	nParticleServer2::Instance()->Trigger();
	PROFILER_STOP(profParticleUpdates);

	PROFILER_START(profRender);
	if (GfxSrv->BeginRender())
	{
		GfxSrv->Render();
		//UISys->Render();
		if (RenderDbgGfx) GfxSrv->RenderDebug();
		if (RenderDbgPhysics) PhysicsSrv->RenderDebug();
		if (RenderDbgEntities) GameSrv->RenderDebug();
		if (RenderDbgAI) AISrv->RenderDebug();
		GfxSrv->EndRender();
	}
	PROFILER_STOP(profRender);

	nKernelServer::Instance()->Trigger();

	// Editor window receives window messages outside the frame, so clear input here, not in the beginning
	InputSrv->Trigger();

	PROFILER_STOP(profCompleteFrame);

	return (Running) ? GetID() : APP_STATE_EXIT;
}
//---------------------------------------------------------------------

bool CAppStateEditor::OnMouseBtnDown(const Events::CEventBase& Event)
{
	if (!CIDEApp->MouseCB) FAIL;
	const Event::MouseBtnDown& Ev = ((const Event::MouseBtnDown&)Event);
	CIDEApp->MouseCB(Ev.X, Ev.Y, (int)Ev.Button, Down);
	OK;
}
//---------------------------------------------------------------------

bool CAppStateEditor::OnMouseBtnUp(const Events::CEventBase& Event)
{
	if (!CIDEApp->MouseCB) FAIL;
	const Event::MouseBtnUp& Ev = ((const Event::MouseBtnUp&)Event);
	this->CIDEApp->MouseCB(Ev.X, Ev.Y, (int)Ev.Button, Up);
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
