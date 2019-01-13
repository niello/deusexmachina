#include "MouseWin32.h"

#include <Input/InputEvents.h>
#include <Events/Subscription.h>
#include <System/Events/OSInput.h>
#include <System/OSWindow.h>

namespace Input
{
CMouseWin32::CMouseWin32() {}
CMouseWin32::~CMouseWin32() {}

bool CMouseWin32::Init(HANDLE hDevice)
{
	RID_DEVICE_INFO DeviceInfo;
	DeviceInfo.cbSize = sizeof(RID_DEVICE_INFO);
	UINT Size = sizeof(RID_DEVICE_INFO);
	if (::GetRawInputDeviceInfo(hDevice, RIDI_DEVICEINFO, &DeviceInfo, &Size) <= 0) FAIL;

	if (DeviceInfo.dwType != RIM_TYPEMOUSE) FAIL;

	char NameBuf[512];
	Size = 512;
	if (::GetRawInputDeviceInfo(hDevice, RIDI_DEVICENAME, NameBuf, &Size) > 0)
	{
		Name = NameBuf;
	}

	ID = DeviceInfo.mouse.dwId;
	_hDevice = hDevice;

	AxisCount = DeviceInfo.mouse.fHasHorizontalWheel ? 4 : 3;
	AxisSensitivity.reset(n_new_array(float, AxisCount));
	for (UPTR i = 0; i < AxisCount; ++i)
	{
		AxisSensitivity[i] = 1.f;
	}

	ButtonCount = DeviceInfo.mouse.dwNumberOfButtons;

	Operational = true;

	OK;
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

bool CMouseWin32::OnOSWindowInput(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	n_assert_dbg(Event.IsA<Event::OSInput>());

	const Event::OSInput& OSInputEvent = (const Event::OSInput&)Event;

	switch (OSInputEvent.Type)
	{
		case Event::OSInput::MouseDown:
		{
			// Button codes received from OSWindow are the same as EMouseButton codes, no conversion needed
			Event::ButtonDown Ev(OSInputEvent.MouseInfo.Button);
			FireEvent(Ev);
			OK;
		}

		case Event::OSInput::MouseUp:
		{
			// Button codes received from OSWindow are the same as EMouseButton codes, no conversion needed
			Event::ButtonUp Ev(OSInputEvent.MouseInfo.Button);
			FireEvent(Ev);
			OK;
		}

		case Event::OSInput::MouseMoveRaw:
		{
			float MoveX = AxisSensitivity[0] * (float)OSInputEvent.MouseInfo.x;
			if (MoveX != 0.f)
			{
				float RelMove = MoveX;// / (float)n_max(pWindow->GetWidth(), 1);
				Event::AxisMove Ev(0, RelMove, MoveX);
				FireEvent(Ev);
			}

			float MoveY = AxisSensitivity[1] * (float)OSInputEvent.MouseInfo.y;
			if (MoveY != 0.f)
			{
				float RelMove = MoveY;// / (float)n_max(pWindow->GetHeight(), 1);
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