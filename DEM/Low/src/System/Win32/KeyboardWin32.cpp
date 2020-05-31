#include "KeyboardWin32.h"

#include <Input/InputEvents.h>
#include <Events/Subscription.h>
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
	if (Handlers.empty()) FAIL;

	const RAWKEYBOARD& KbData = Data.data.keyboard;

	// discard "fake keys" which are part of an escaped sequence
	if (KbData.VKey == KEYBOARD_OVERRUN_MAKE_CODE) FAIL;

	// e0 and e1 are escape sequences used for certain special keys, such as PRINT and PAUSE/BREAK.
	// see http://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
	const bool IsE0 = ((KbData.Flags & RI_KEY_E0) != 0);
	const bool IsE1 = ((KbData.Flags & RI_KEY_E1) != 0);

	const UINT VKey = KbData.VKey;
	UINT ScanCode = KbData.MakeCode;
	U8 ResultCode = EKey::Key_Invalid;

	if (VKey == VK_NUMLOCK)
	{
		// correct PAUSE/BREAK and NUM LOCK silliness, and set the extended bit
		ScanCode = (::MapVirtualKey(VKey, MAPVK_VK_TO_VSC) | 0x100);
	}

	if (IsE1)
	{
		// for escaped sequences, turn the virtual key into the correct scan code using MapVirtualKey.
		// however, MapVirtualKey is unable to map VK_PAUSE (this is a known bug), hence we map that by hand.
		if (VKey == VK_PAUSE) ScanCode = 0x45;
		else ScanCode = ::MapVirtualKey(VKey, MAPVK_VK_TO_VSC);
	}

	switch (VKey)
	{
		case VK_LWIN:		ResultCode = EKey::LeftWindows; break;
		case VK_RWIN:		ResultCode = EKey::RightWindows; break;
		case VK_APPS:		ResultCode = EKey::AppMenu; break;
		case VK_PAUSE:		ResultCode = EKey::Pause; break;
		case VK_NUMLOCK:	ResultCode = EKey::NumLock; break;
		case VK_SNAPSHOT:	ResultCode = EKey::PrintScreen; break;
		case VK_DIVIDE:		ResultCode = EKey::Divide; break;

		case VK_SHIFT:
		{
			// correct left-hand / right-hand SHIFT
			const UINT VKeyShift = ::MapVirtualKey(ScanCode, MAPVK_VSC_TO_VK_EX);
			ResultCode = (VKeyShift == VK_RSHIFT) ? EKey::RightShift : EKey::LeftShift;
			break;
		}

		// right-hand CONTROL and ALT have their e0 bit set
		case VK_CONTROL:	ResultCode = IsE0 ? EKey::RightControl : EKey::LeftControl; break;
		case VK_MENU:		ResultCode = IsE0 ? EKey::RightAlt : EKey::LeftAlt; break;

		// NUMPAD ENTER has its e0 bit set
		case VK_RETURN:		ResultCode = IsE0 ? EKey::NumpadEnter : EKey::Enter; break;

		// the standard INSERT, DELETE, HOME, END, PRIOR and NEXT keys will always have their e0 bit set, but the
		// corresponding keys on the NUMPAD will not.
		case VK_INSERT:		ResultCode = IsE0 ? EKey::Insert : EKey::Numpad0; break;
		case VK_DELETE:		ResultCode = IsE0 ? EKey::Delete : EKey::Decimal; break;
		case VK_HOME:		ResultCode = IsE0 ? EKey::Home : EKey::Numpad7; break;
		case VK_END:		ResultCode = IsE0 ? EKey::End : EKey::Numpad1; break;
		case VK_PRIOR:		ResultCode = IsE0 ? EKey::PageUp : EKey::Numpad9; break;
		case VK_NEXT:		ResultCode = IsE0 ? EKey::PageDown : EKey::Numpad3; break;

		// the standard arrow keys will always have their e0 bit set, but the
		// corresponding keys on the NUMPAD will not.
		case VK_LEFT:		ResultCode = IsE0 ? EKey::ArrowLeft : EKey::Numpad4; break;
		case VK_RIGHT:		ResultCode = IsE0 ? EKey::ArrowRight : EKey::Numpad6; break;
		case VK_UP:			ResultCode = IsE0 ? EKey::ArrowUp : EKey::Numpad8; break;
		case VK_DOWN:		ResultCode = IsE0 ? EKey::ArrowDown : EKey::Numpad2; break;

		// NUMPAD 5 doesn't have its e0 bit set
		case VK_CLEAR:		if (!IsE0) ResultCode = EKey::Numpad5; break;

		default:			ResultCode = ScanCode; break;
	}

	/* Correct human-readable key names:
	LONG KeyForText = (ScanCode << 16) | (IsE0 << 24);
	char Buffer[512] = {};
	::GetKeyNameText(KeyForText, Buffer, 512);
	::Sys::DbgOut(CString("CKeyboardWin32::HandleRawInput(") + StringUtils::FromInt(KbData.Message) + ") " + Buffer + ((KbData.Flags & RI_KEY_BREAK) ? " up\n" : " down\n"));
	*/

	if (ResultCode == EKey::Key_Invalid || !ResultCode) FAIL;

	if (KbData.Flags & RI_KEY_BREAK)
		return FireEvent(Event::ButtonUp(this, ResultCode), Events::Event_TermOnHandled) > 0;
	else
		return FireEvent(Event::ButtonDown(this, ResultCode), Events::Event_TermOnHandled) > 0;
}
//---------------------------------------------------------------------

bool CKeyboardWin32::HandleCharMessage(WPARAM Char)
{
	//WCHAR CharUTF16[2];
	//::MultiByteToWideChar(CP_ACP, 0, (const char*)&Char, 1, CharUTF16, 1);

	std::string Text(1, static_cast<char>(Char));
	return FireEvent(Event::TextInput(this, std::move(Text), true), Events::Event_TermOnHandled);
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
