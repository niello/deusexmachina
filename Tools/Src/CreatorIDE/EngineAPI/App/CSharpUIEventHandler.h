#pragma once
#ifndef __CIDE_CSHARP_UI_EVENT_HANDLER_H__
#define __CIDE_CSHARP_UI_EVENT_HANDLER_H__

#include <App/EditorTool.h>
#include <Events/Events.h>

// Communication point between editor API and UI. Transfers editor events to the WinForms UI.

enum EMouseAction
{
    Down = 0,
    Click,
    Up,
    DoubleClick
};

typedef void (__stdcall *CCallback_V_V)();
typedef void (__stdcall *CCallback_V_S)(const char*);
typedef void (__stdcall *CMouseButtonCallback)(int x, int y, int Button, EMouseAction Action);

namespace App
{

class CCSharpUIEventHandler
{
protected:

	DECLARE_EVENT_HANDLER(OnEntitySelected, OnEntitySelected);
	DECLARE_EVENT_HANDLER(MouseBtnDown, OnMouseBtnDown);
	DECLARE_EVENT_HANDLER(MouseBtnUp, OnMouseBtnUp);

public:

	// Now communication with C# is done through hand-written unmanaged->managed code callbacks
	CCallback_V_S			OnEntitySelectedCB;
	CMouseButtonCallback	MouseButtonCB;

	CCSharpUIEventHandler();
};

}

#endif
