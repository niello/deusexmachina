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
	ButtonCount = EKey::Key_Count; // We want a virtual key space, not a physical key count from DeviceInfo.dwNumberOfKeysTotal
	Operational = true;
	OK;
}
//---------------------------------------------------------------------

// Logic is taken almost as is from: https://blog.molecular-matters.com/2011/09/05/properly-handling-keyboard-input/
// Thanks a lot! Comments are preserved for clarity.
bool CKeyboardWin32::HandleRawInput(const RAWINPUT& Data)
{
	if (Data.header.dwType != RIM_TYPEKEYBOARD) FAIL;

	// No one reads our input
	if (Subscriptions.IsEmpty()) FAIL;

	const RAWKEYBOARD& KbData = Data.data.keyboard;

	UINT VirtualKey = KbData.VKey;
	UINT ScanCode = KbData.MakeCode;

	// e0 and e1 are escape sequences used for certain special keys, such as PRINT and PAUSE/BREAK.
	// see http://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
	const bool IsE0 = ((KbData.Flags & RI_KEY_E0) != 0);
	switch (VirtualKey)
	{
		// discard "fake keys" which are part of an escaped sequence
		case 255:			FAIL;

		// correct left-hand / right-hand SHIFT
		case VK_SHIFT:		VirtualKey = ::MapVirtualKey(ScanCode, MAPVK_VSC_TO_VK_EX); break;

		// correct PAUSE/BREAK and NUM LOCK silliness, and set the extended bit
		case VK_NUMLOCK:	ScanCode = (::MapVirtualKey(VirtualKey, MAPVK_VK_TO_VSC) | 0x100); break;

		// right-hand CONTROL and ALT have their e0 bit set
		case VK_CONTROL:	VirtualKey = IsE0 ? VK_RCONTROL : VK_LCONTROL; break;
		case VK_MENU:		VirtualKey = IsE0 ? VK_RMENU : VK_LMENU; break;

		// NUMPAD ENTER has its e0 bit set
		case VK_RETURN:		if (IsE0) VirtualKey = VK_SEPARATOR; break; // VK_SEPARATOR is not a numpad Enter but we use it as such

		// the standard INSERT, DELETE, HOME, END, PRIOR and NEXT keys will always have their e0 bit set, but the
		// corresponding keys on the NUMPAD will not.
		case VK_INSERT:		if (!IsE0) VirtualKey = VK_NUMPAD0; break;
		case VK_DELETE:		if (!IsE0) VirtualKey = VK_DECIMAL; break;
		case VK_HOME:		if (!IsE0) VirtualKey = VK_NUMPAD7; break;
		case VK_END:		if (!IsE0) VirtualKey = VK_NUMPAD1; break;
		case VK_PRIOR:		if (!IsE0) VirtualKey = VK_NUMPAD9; break;
		case VK_NEXT:		if (!IsE0) VirtualKey = VK_NUMPAD3; break;

		// the standard arrow keys will always have their e0 bit set, but the
		// corresponding keys on the NUMPAD will not.
		case VK_LEFT:		if (!IsE0) VirtualKey = VK_NUMPAD4; break;
		case VK_RIGHT:		if (!IsE0) VirtualKey = VK_NUMPAD6; break;
		case VK_UP:			if (!IsE0) VirtualKey = VK_NUMPAD8; break;
		case VK_DOWN:		if (!IsE0) VirtualKey = VK_NUMPAD2; break;

		// NUMPAD 5 doesn't have its e0 bit set
		case VK_CLEAR:		if (!IsE0) VirtualKey = VK_NUMPAD5; break;
	}

	if (KbData.Flags & RI_KEY_E1)
	{
		// for escaped sequences, turn the virtual key into the correct scan code using MapVirtualKey.
		// however, MapVirtualKey is unable to map VK_PAUSE (this is a known bug), hence we map that by hand.
		if (VirtualKey == VK_PAUSE) ScanCode = 0x45;
		else ScanCode = ::MapVirtualKey(VirtualKey, MAPVK_VK_TO_VSC);
	}

	const bool IsKeyUp = ((KbData.Flags & RI_KEY_BREAK) != 0);

	//!!!DBG TMP!
	UINT key = (ScanCode << 16) | (IsE0 << 24);
	char buffer[512] = {};
	::GetKeyNameText((LONG)key, buffer, 512);
	::Sys::DbgOut(CString("CKeyboardWin32::HandleRawInput() ") + buffer + (IsKeyUp ? " up\n" : " down\n"));

	FAIL;
}
//---------------------------------------------------------------------

U8 CKeyboardWin32::GetButtonCode(const char* pAlias) const
{
	// Typical codes and names are used
	EKey Key = StringToKey(pAlias);
	return (Key == Key_Invalid) ? InvalidCode : (U8)Key;
}
//---------------------------------------------------------------------

const char* CKeyboardWin32::GetButtonAlias(U8 Code) const
{
	//!!! :: GetKeyNameText !

	// Typical codes and names are used
	return KeyToString((EKey)Code);
}
//---------------------------------------------------------------------

}