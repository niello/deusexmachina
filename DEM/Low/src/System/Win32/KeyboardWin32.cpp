#include "KeyboardWin32.h"

#include <Input/InputEvents.h>
#include <Events/Subscription.h>
#include <System/Events/OSInput.h>
#include <System/OSWindow.h>

namespace Input
{
CKeyboardWin32::CKeyboardWin32() {}
CKeyboardWin32::~CKeyboardWin32() {}

bool CKeyboardWin32::Init(HANDLE hDevice)
{
	RID_DEVICE_INFO DeviceInfo;
	DeviceInfo.cbSize = sizeof(RID_DEVICE_INFO);
	UINT Size = sizeof(RID_DEVICE_INFO);
	if (::GetRawInputDeviceInfo(hDevice, RIDI_DEVICEINFO, &DeviceInfo, &Size) <= 0) FAIL;

	if (DeviceInfo.dwType != RIM_TYPEKEYBOARD) FAIL;

	char NameBuf[512];
	Size = 512;
	if (::GetRawInputDeviceInfo(hDevice, RIDI_DEVICENAME, NameBuf, &Size) > 0)
	{
		Name = NameBuf;
	}

	Type = DeviceInfo.keyboard.dwType;
	Subtype = DeviceInfo.keyboard.dwSubType;
	_hDevice = hDevice;

	ButtonCount = DeviceInfo.keyboard.dwNumberOfKeysTotal;

	Operational = true;

	OK;
}
//---------------------------------------------------------------------

U8 CKeyboardWin32::GetButtonCode(const char* pAlias) const
{
	//!!!IMPLEMENT!
	return InvalidCode;
}
//---------------------------------------------------------------------

const char* CKeyboardWin32::GetButtonAlias(U8 Code) const
{
	//!!!IMPLEMENT!
	return nullptr;
}
//---------------------------------------------------------------------

//!!!Current approach is temporary and not robust across different keyboards and platforms!
static inline U8 ConvertToKeyCode(U8 ScanCode, U8 VirtualKey, bool IsExtended)
{
	//!!!IMPLEMENT PROPERLY!
	U8 KeyCode = ScanCode;
	if (IsExtended) KeyCode |= 0x80;
	return KeyCode;
}
//---------------------------------------------------------------------

bool CKeyboardWin32::OnOSWindowInput(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	n_assert_dbg(Event.IsA<Event::OSInput>());

	const Event::OSInput& OSInputEvent = (const Event::OSInput&)Event;

	switch (OSInputEvent.Type)
	{
		case Event::OSInput::KeyDown:
		{
			// We are interested only in actual key down events
			if (!(OSInputEvent.KeyboardInfo.Flags & Event::OSInput::Key_Repeated))
			{
				U8 KeyCode = ConvertToKeyCode(
					OSInputEvent.KeyboardInfo.ScanCode,
					OSInputEvent.KeyboardInfo.VirtualKey,
					(OSInputEvent.KeyboardInfo.Flags & Event::OSInput::Key_Extended) != 0);
				Event::ButtonDown Ev(KeyCode);
				FireEvent(Ev);
			}
			OK;
		}

		case Event::OSInput::KeyUp:
		{
			U8 KeyCode = ConvertToKeyCode(
				OSInputEvent.KeyboardInfo.ScanCode,
				OSInputEvent.KeyboardInfo.VirtualKey,
				(OSInputEvent.KeyboardInfo.Flags & Event::OSInput::Key_Extended) != 0);
			Event::ButtonUp Ev(KeyCode);
			FireEvent(Ev);
			OK;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

}