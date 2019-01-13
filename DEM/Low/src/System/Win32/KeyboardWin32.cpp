#include "KeyboardWin32.h"

#include <Input/InputEvents.h>
#include <Events/Subscription.h>
#include <System/Events/OSInput.h>
#include <System/OSWindow.h>

namespace Input
{
CKeyboardWin32::CKeyboardWin32() {}
CKeyboardWin32::~CKeyboardWin32() {}

bool CKeyboardWin32::Init(HANDLE hDevice, const CString& DeviceName, const RID_DEVICE_INFO_KEYBOARD& DeviceInfo)
{
	Name = DeviceName;
	Type = DeviceInfo.dwType;
	Subtype = DeviceInfo.dwSubType;
	_hDevice = hDevice;
	ButtonCount = DeviceInfo.dwNumberOfKeysTotal;
	Operational = true;
	OK;
}
//---------------------------------------------------------------------

bool CKeyboardWin32::HandleRawInput(const RAWINPUT& Data)
{
	::Sys::DbgOut("CKeyboardWin32::HandleRawInput()\n");
	FAIL;
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