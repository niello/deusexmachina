#include "MouseWin32.h"

#include <Input/InputEvents.h>
#include <Events/Subscription.h>
#include <System/Events/OSInput.h>
#include <System/OSWindow.h>

#define RI_MOUSE_HWHEEL 0x0800

namespace Input
{
CMouseWin32::CMouseWin32() {}
CMouseWin32::~CMouseWin32() {}

bool CMouseWin32::Init(HANDLE hDevice, const CString& DeviceName, const RID_DEVICE_INFO_MOUSE& DeviceInfo)
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

bool CMouseWin32::HandleRawInput(const RAWINPUT& Data)
{
	if (Data.header.dwType != RIM_TYPEMOUSE) FAIL;

	// No one reads our input
	if (Subscriptions.IsEmpty()) FAIL;

	const RAWMOUSE& MouseData = Data.data.mouse;

	// MouseData.usFlags & MOUSE_ATTRIBUTES_CHANGED - must query mouse attributes. Axis & button count might change?

	// TODO: process it correctly? or never happens?
	n_assert(!(MouseData.usFlags & MOUSE_MOVE_ABSOLUTE) && !(MouseData.usFlags & MOUSE_VIRTUAL_DESKTOP));

	if (MouseData.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) FireEvent(Event::ButtonDown(0));
	if (MouseData.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) FireEvent(Event::ButtonUp(0));
	if (MouseData.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) FireEvent(Event::ButtonDown(1));
	if (MouseData.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) FireEvent(Event::ButtonUp(1));
	if (MouseData.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) FireEvent(Event::ButtonDown(2));
	if (MouseData.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP) FireEvent(Event::ButtonUp(2));
	if (MouseData.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) FireEvent(Event::ButtonDown(3));
	if (MouseData.usButtonFlags & RI_MOUSE_BUTTON_4_UP) FireEvent(Event::ButtonUp(3));
	if (MouseData.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) FireEvent(Event::ButtonDown(4));
	if (MouseData.usButtonFlags & RI_MOUSE_BUTTON_5_UP) FireEvent(Event::ButtonUp(4));

	const float MoveX = AxisSensitivity[0] * MouseData.lLastX;
	if (MoveX != 0.f) FireEvent(Event::AxisMove(0, MoveX));

	const float MoveY = AxisSensitivity[1] * MouseData.lLastY;
	if (MoveY != 0.f) FireEvent(Event::AxisMove(1, MoveY));

	if (MouseData.usButtonFlags & RI_MOUSE_WHEEL)
	{
		const float Move = AxisSensitivity[2] * (static_cast<SHORT>(MouseData.usButtonData) / WHEEL_DELTA);
		if (Move != 0.f) FireEvent(Event::AxisMove(2, Move));
	}

	if (MouseData.usButtonFlags & RI_MOUSE_HWHEEL)
	{
		const UPTR AxisIndex = (AxisCount > 3) ? 3 : 2;
		const float Move = AxisSensitivity[AxisIndex] * (static_cast<SHORT>(MouseData.usButtonData) / WHEEL_DELTA);
		if (Move != 0.f) FireEvent(Event::AxisMove(AxisIndex, Move));
	}

	OK; //???or return false if no messages were processed?
}
//---------------------------------------------------------------------

// TODO: system-wide aliases for typical controls!

U8 CMouseWin32::GetAxisCode(const char* pAlias) const
{
	if (!strcmp(pAlias, "Mouse_Axis_X")) return 0;
	if (!strcmp(pAlias, "Mouse_Axis_Y")) return 1;
	if (!strcmp(pAlias, "Mouse_Axis_Wheel")) return 2;
	if (AxisCount > 3 && !strcmp(pAlias, "Mouse_Axis_HWheel")) return 3;
	return InvalidCode;
}
//---------------------------------------------------------------------

const char* CMouseWin32::GetAxisAlias(U8 Code) const
{
	switch (Code)
	{
		case 0: return "Mouse_Axis_X";
		case 1: return "Mouse_Axis_Y";
		case 2: return "Mouse_Axis_Wheel";
		case 3: return (AxisCount > 3) ? "Mouse_Axis_HWheel" : nullptr;
	}
	return nullptr;
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
	EMouseButton Button = StringToMouseButton(pAlias);
	return (Button == MB_Invalid) ? InvalidCode : (U8)Button;
}
//---------------------------------------------------------------------

const char* CMouseWin32::GetButtonAlias(U8 Code) const
{
	return MouseButtonToString((EMouseButton)Code);
}
//---------------------------------------------------------------------

}