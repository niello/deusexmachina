#pragma once
#ifndef __CIDE_APP_STATE_EDITOR_H__
#define __CIDE_APP_STATE_EDITOR_H__

#include <App/StateHandler.h>
#include <App/ToolSelect.h>
#include <App/ToolTransform.h>
#include <App/ToolNavRegions.h>

// This handler renders scene and debug visuals, updates low-level game state
// and manages editor tools.

namespace App
{

class CAppStateEditor: public CStateHandler
{
	DeclareRTTI;

protected:

	CToolSelect		ToolSelect;
	CToolTransform	ToolTransform;
	CToolNavRegions	ToolNavRegions;
	IEditorTool*	pActiveTool;

	bool			RenderDbgAI;
	bool			RenderDbgPhysics;
	bool			RenderDbgGfx;
	bool			RenderDbgEntities;

	DECLARE_EVENT_HANDLER(ToggleGamePause, OnToggleGamePause);
	DECLARE_EVENT_HANDLER(ToggleRenderDbgAI, OnToggleRenderDbgAI);
	DECLARE_EVENT_HANDLER(ToggleRenderDbgPhysics, OnToggleRenderDbgPhysics);
	DECLARE_EVENT_HANDLER(ToggleRenderDbgGfx, OnToggleRenderDbgGfx);
	DECLARE_EVENT_HANDLER(ToggleRenderDbgScene, OnToggleRenderDbgScene);
	DECLARE_EVENT_HANDLER(ToggleRenderDbgEntities, OnToggleRenderDbgEntities);

public:

	CAppStateEditor(CStrID StateID);
	virtual ~CAppStateEditor() {}

	virtual void	OnStateEnter(CStrID PrevState, PParams Params = NULL);
	virtual void	OnStateLeave(CStrID NextState);
	virtual CStrID	OnFrame();

	bool			SetTool(LPCSTR Name); //!!!RTTI!
};

//RegisterFactory(CAppStateEditor);

}

#endif
