#include "OSWindowMouse.h"

#include <Input/InputEvents.h>
#include <System/Events/OSInput.h>
#include <System/OSWindow.h>

namespace Input
{

void COSWindowMouse::Attach(Sys::COSWindow* pOSWindow, U16 Priority)
{
	if (pOSWindow)
	{
		DISP_SUBSCRIBE_NEVENT_PRIORITY(pOSWindow, OSInput, COSWindowMouse, OnOSWindowInput, Priority);
	}
	else
	{
		UNSUBSCRIBE_EVENT(OSInput);
	}
}
//---------------------------------------------------------------------

U8 COSWindowMouse::GetAxisCode(const char* pAlias) const
{
	if (!n_stricmp(pAlias, "x")) return 0;
	if (!n_stricmp(pAlias, "y")) return 1;
	if (!n_stricmp(pAlias, "wheel1")) return 2;
	if (!n_stricmp(pAlias, "wheel2")) return 3;
	return InvalidCode;
}
//---------------------------------------------------------------------

const char* COSWindowMouse::GetAxisAlias(U8 Code) const
{
	switch (Code)
	{
		case 0:		return "X";
		case 1:		return "Y";
		case 2:		return "Wheel1";
		case 3:		return "Wheel2";
		default:	return NULL;
	}
}
//---------------------------------------------------------------------

U8 COSWindowMouse::GetButtonCode(const char* pAlias) const
{
	if (!n_stricmp(pAlias, "lmb")) return 0;
	if (!n_stricmp(pAlias, "rmb")) return 1;
	if (!n_stricmp(pAlias, "mmb")) return 2;
	if (!n_stricmp(pAlias, "x1")) return 3;
	if (!n_stricmp(pAlias, "x2")) return 4;
	if (!n_stricmp(pAlias, "other")) return 5;
	return InvalidCode;
}
//---------------------------------------------------------------------

const char* COSWindowMouse::GetButtonAlias(U8 Code) const
{
	switch (Code)
	{
		case 0:		return "LMB";
		case 1:		return "RMB";
		case 2:		return "MMB";
		case 3:		return "X1";
		case 4:		return "X2";
		case 5:		return "Other";
		default:	return NULL;
	}
}
//---------------------------------------------------------------------

bool COSWindowMouse::OnOSWindowInput(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	n_assert_dbg(Event.IsA<Event::OSInput>());

	const Event::OSInput& OSInputEvent = (const Event::OSInput&)Event;

	switch (OSInputEvent.Type)
	{
		case Event::OSInput::MouseDown:
		{
			//???use queue and store until requiested? user will pick events, only one user is normally possible
			//???!!!not event, just a special structure?! or use async events?
			Event::ButtonDown Ev(OSInputEvent.MouseInfo.Button);
			break;
		}

		case Event::OSInput::MouseUp:
		{
			Event::ButtonUp Ev(OSInputEvent.MouseInfo.Button);
			break;
		}

		//case Event::OSInput::MouseMove:
		//case Event::OSInput::MouseMoveRaw:

		case Event::OSInput::MouseWheelVertical:

		case Event::OSInput::MouseWheelHorizontal:

		default: FAIL;
	}

	FAIL;
}
//---------------------------------------------------------------------

}