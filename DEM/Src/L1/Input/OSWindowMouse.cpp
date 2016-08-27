#include "OSWindowMouse.h"

#include <Input/InputEvents.h>
#include <System/Events/OSInput.h>
#include <System/OSWindow.h>

namespace Input
{

COSWindowMouse::COSWindowMouse(): pWindow(NULL)
{
	AxisSensitivity[0] = 1.f;
	AxisSensitivity[1] = 1.f;
	AxisSensitivity[2] = 1.f;
	AxisSensitivity[3] = 1.f;
}
//---------------------------------------------------------------------

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
	pWindow = pOSWindow;
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

void COSWindowMouse::SetAxisSensitivity(U8 Code, float Sensitivity)
{
	n_assert(Code < AxisCount);
	AxisSensitivity[Code] = Sensitivity;
}
//---------------------------------------------------------------------

float COSWindowMouse::GetAxisSensitivity(U8 Code) const
{
	n_assert(Code < AxisCount);
	return AxisSensitivity[Code];
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
			Event::ButtonDown Ev(OSInputEvent.MouseInfo.Button);
			FireEvent(Ev);
			OK;
		}

		case Event::OSInput::MouseUp:
		{
			Event::ButtonUp Ev(OSInputEvent.MouseInfo.Button);
			FireEvent(Ev);
			OK;
		}

		case Event::OSInput::MouseMoveRaw:
		{
			float MoveX = AxisSensitivity[0] * (float)OSInputEvent.MouseInfo.x;
			if (MoveX != 0.f)
			{
				float RelMove = MoveX / (float)n_max(pWindow->GetWidth(), 1);
				Event::AxisMove Ev(0, RelMove, MoveX);
				FireEvent(Ev);
			}

			float MoveY = AxisSensitivity[1] * (float)OSInputEvent.MouseInfo.y;
			if (MoveY != 0.f)
			{
				float RelMove = MoveY / (float)n_max(pWindow->GetHeight(), 1);
				Event::AxisMove Ev(1, RelMove, MoveY);
				FireEvent(Ev);
			}

			OK;
		}

		case Event::OSInput::MouseWheelVertical:
		{
			float Move = AxisSensitivity[2] * (float)OSInputEvent.WheelDelta;
			Event::AxisMove Ev(2, Move, Move);
			FireEvent(Ev);
			OK;
		}

		case Event::OSInput::MouseWheelHorizontal:
		{
			float Move = AxisSensitivity[2] * (float)OSInputEvent.WheelDelta;
			Event::AxisMove Ev(3, Move, Move);
			FireEvent(Ev);
			OK;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

}