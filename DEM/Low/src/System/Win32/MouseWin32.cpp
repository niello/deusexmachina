#include "MouseWin32.h"

#include <Input/InputEvents.h>

#define RI_MOUSE_HWHEEL 0x0800

namespace Input
{
CMouseWin32::CMouseWin32() {}
CMouseWin32::~CMouseWin32() {}

bool CMouseWin32::Init(HANDLE hDevice, const std::string& DeviceName, const RID_DEVICE_INFO_MOUSE& DeviceInfo)
{
	Name = DeviceName;
	ID = DeviceInfo.dwId;
	_hDevice = hDevice;

	AxisCount = DeviceInfo.fHasHorizontalWheel ? 4 : 3;
	AxisSensitivity.reset(n_new_array(float, AxisCount));
	for (UPTR i = 0; i < AxisCount; ++i)
	{
		AxisSensitivity[i] = 1.f;
	}

	ButtonCount = DeviceInfo.dwNumberOfButtons;

	Operational = true;

	OK;
}
//---------------------------------------------------------------------

bool CMouseWin32::CanHandleRawInput(const RAWINPUT& Data) const
{
	if (Data.header.dwType != RIM_TYPEMOUSE) return false;

	const RAWMOUSE& MouseData = Data.data.mouse;
	const U32 RequiredButtonCount =
		(MouseData.usButtonFlags & (RI_MOUSE_BUTTON_5_DOWN | RI_MOUSE_BUTTON_5_UP)) ? 5 :
		(MouseData.usButtonFlags & (RI_MOUSE_BUTTON_4_DOWN | RI_MOUSE_BUTTON_4_UP)) ? 4 :
		(MouseData.usButtonFlags & (RI_MOUSE_MIDDLE_BUTTON_DOWN | RI_MOUSE_MIDDLE_BUTTON_UP)) ? 3 :
		(MouseData.usButtonFlags & (RI_MOUSE_RIGHT_BUTTON_DOWN | RI_MOUSE_RIGHT_BUTTON_UP)) ? 2 :
		(MouseData.usButtonFlags & (RI_MOUSE_LEFT_BUTTON_DOWN | RI_MOUSE_LEFT_BUTTON_UP)) ? 1 : 0;
	if (ButtonCount < RequiredButtonCount) return false;

	const U32 RequiredAxisCount =
		(MouseData.usButtonFlags & (RI_MOUSE_HWHEEL | RI_MOUSE_WHEEL)) ? 3 :
		MouseData.lLastY ? 2 :
		MouseData.lLastX ? 1 : 0;
	return AxisCount >= RequiredAxisCount;
}
//---------------------------------------------------------------------

bool CMouseWin32::HandleRawInput(const RAWINPUT& Data)
{
	if (Data.header.dwType != RIM_TYPEMOUSE) FAIL;

	// No one reads our input
	if (Handlers.empty()) FAIL;

	const RAWMOUSE& MouseData = Data.data.mouse;

	// MouseData.usFlags & MOUSE_ATTRIBUTES_CHANGED - must query mouse attributes. Axis & button count might change?

	// TODO: process it correctly? or never happens?
	n_assert(!(MouseData.usFlags & MOUSE_MOVE_ABSOLUTE) && !(MouseData.usFlags & MOUSE_VIRTUAL_DESKTOP));

	UPTR HandledCount = 0;
	if (MouseData.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
		HandledCount += FireEvent(Event::ButtonDown(this, 0), Events::Event_TermOnHandled);
	if (MouseData.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
		HandledCount += FireEvent(Event::ButtonUp(this, 0), Events::Event_TermOnHandled);
	if (MouseData.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
		HandledCount += FireEvent(Event::ButtonDown(this, 1), Events::Event_TermOnHandled);
	if (MouseData.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
		HandledCount += FireEvent(Event::ButtonUp(this, 1), Events::Event_TermOnHandled);
	if (MouseData.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
		HandledCount += FireEvent(Event::ButtonDown(this, 2), Events::Event_TermOnHandled);
	if (MouseData.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
		HandledCount += FireEvent(Event::ButtonUp(this, 2), Events::Event_TermOnHandled);
	if (MouseData.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN)
		HandledCount += FireEvent(Event::ButtonDown(this, 3), Events::Event_TermOnHandled);
	if (MouseData.usButtonFlags & RI_MOUSE_BUTTON_4_UP)
		HandledCount += FireEvent(Event::ButtonUp(this, 3), Events::Event_TermOnHandled);
	if (MouseData.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN)
		HandledCount += FireEvent(Event::ButtonDown(this, 4), Events::Event_TermOnHandled);
	if (MouseData.usButtonFlags & RI_MOUSE_BUTTON_5_UP)
		HandledCount += FireEvent(Event::ButtonUp(this, 4), Events::Event_TermOnHandled);

	const float MoveX = AxisSensitivity[0] * MouseData.lLastX;
	if (MoveX != 0.f)
		HandledCount += FireEvent(Event::AxisMove(this, 0, MoveX), Events::Event_TermOnHandled);

	const float MoveY = AxisSensitivity[1] * MouseData.lLastY;
	if (MoveY != 0.f)
		HandledCount += FireEvent(Event::AxisMove(this, 1, MoveY), Events::Event_TermOnHandled);

	if (MouseData.usButtonFlags & RI_MOUSE_WHEEL)
	{
		const float Move = AxisSensitivity[2] * (static_cast<SHORT>(MouseData.usButtonData) / WHEEL_DELTA);
		if (Move != 0.f)
			HandledCount += FireEvent(Event::AxisMove(this, 2, Move), Events::Event_TermOnHandled);
	}

	if (MouseData.usButtonFlags & RI_MOUSE_HWHEEL)
	{
		const UPTR AxisIndex = (AxisCount > 3) ? 3 : 2;
		const float Move = AxisSensitivity[AxisIndex] * (static_cast<SHORT>(MouseData.usButtonData) / WHEEL_DELTA);
		if (Move != 0.f)
			HandledCount += FireEvent(Event::AxisMove(this, AxisIndex, Move), Events::Event_TermOnHandled);
	}

	return HandledCount > 0;
}
//---------------------------------------------------------------------

U8 CMouseWin32::GetAxisCode(const char* pAlias) const
{
	// Typical codes and names are used
	EMouseAxis Axis = StringToMouseAxis(pAlias);
	return (Axis == MouseAxis_Invalid) ? InvalidCode : (U8)Axis;
}
//---------------------------------------------------------------------

const char* CMouseWin32::GetAxisAlias(U8 Code) const
{
	// Typical codes and names are used
	return MouseAxisToString((EMouseAxis)Code);
}
//---------------------------------------------------------------------

void CMouseWin32::SetAxisSensitivity(U8 Code, float Sensitivity)
{
	if (Code < AxisCount) AxisSensitivity[Code] = Sensitivity;
}
//---------------------------------------------------------------------

float CMouseWin32::GetAxisSensitivity(U8 Code) const
{
	return (Code < AxisCount) ? AxisSensitivity[Code] : 0.f;
}
//---------------------------------------------------------------------

U8 CMouseWin32::GetButtonCode(const char* pAlias) const
{
	// Typical codes and names are used
	EMouseButton Button = StringToMouseButton(pAlias);
	return (Button == MouseButton_Invalid) ? InvalidCode : (U8)Button;
}
//---------------------------------------------------------------------

const char* CMouseWin32::GetButtonAlias(U8 Code) const
{
	// Typical codes and names are used
	return MouseButtonToString((EMouseButton)Code);
}
//---------------------------------------------------------------------

}
