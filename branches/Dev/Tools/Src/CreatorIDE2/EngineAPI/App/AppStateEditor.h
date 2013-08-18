#pragma once
#ifndef __CIDE_APP_STATE_EDITOR_H__
#define __CIDE_APP_STATE_EDITOR_H__

#include <App/StateHandler.h>
#include "CIDEApp.h"

// The game state handler runs the game loop.

namespace App
{

class CAppStateEditor: public CStateHandler
{
	__DeclareClassNoFactory
	//DeclareFactory(CAppStateEditor);

protected:

	bool		RenderDbgAI;
	bool		RenderDbgPhysics;
	bool		RenderDbgGfx;
	bool		RenderDbgEntities;
	PCIDEApp	CIDEApp;

	/*PROFILER_DECLARE(profCompleteFrame);
	PROFILER_DECLARE(profParticleUpdates);
	PROFILER_DECLARE(profRender);*/

	DECLARE_EVENT_HANDLER(MouseBtnDown, OnMouseBtnDown);
	DECLARE_EVENT_HANDLER(MouseBtnUp, OnMouseBtnUp);
	DECLARE_EVENT_HANDLER(ToggleGamePause, OnToggleGamePause);
	DECLARE_EVENT_HANDLER(ToggleRenderDbgAI, OnToggleRenderDbgAI);
	DECLARE_EVENT_HANDLER(ToggleRenderDbgPhysics, OnToggleRenderDbgPhysics);
	DECLARE_EVENT_HANDLER(ToggleRenderDbgGfx, OnToggleRenderDbgGfx);
	DECLARE_EVENT_HANDLER(ToggleRenderDbgScene, OnToggleRenderDbgScene);
	DECLARE_EVENT_HANDLER(ToggleRenderDbgEntities, OnToggleRenderDbgEntities);

public:

	CAppStateEditor(CStrID StateID, PCIDEApp pApp);
	virtual ~CAppStateEditor();

	virtual void	OnStateEnter(CStrID PrevState, PParams Params = NULL);
	virtual void	OnStateLeave(CStrID NextState);
	virtual CStrID	OnFrame();
};

//RegisterFactory(CAppStateEditor);

}

#endif
