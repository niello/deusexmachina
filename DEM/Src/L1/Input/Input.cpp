#include "InputFwd.h"

namespace Input
{

static const char* pDeviceTypeString[] =
{
	"Keyboard",
	"Mouse",
	"Gamepad"
};

static const char* pMouseAxisString[] =
{
	"X",
	"Y",
	"Wheel1",
	"Wheel2"
};

static const char* pMouseButtonString[] =
{
	"LMB",
	"RMB",
	"MMB",
	"X1",
	"X2",
	"User"
};

static const char* pKeyString[] =
{
	"Escape",
	"One",
	"Two",
	"Three",
	"Four",
	"Five",
	"Six",
	"Seven",
	"Eight",
	"Nine",
	"Zero",
	"Minus",
	"Equals",
	"Backspace",
	"Tab",
	"Q",
	"W",
	"E",
	"R",
	"T",
	"Y",
	"U",
	"I",
	"O",
	"P",
	"LeftBracket",
	"RightBracket",
	"Return",
	"LeftControl",
	"A",
	"S",
	"D",
	"F",
	"G",
	"H",
	"J",
	"K",
	"L",
	"Semicolon",
	"Apostrophe",
	"Grave",
	"LeftShift",
	"Backslash",
	"Z",
	"X",
	"C",
	"V",
	"B",
	"N",
	"M",
	"Comma",
	"Period",
	"Slash",
	"RightShift",
	"Multiply",
	"LeftAlt",
	"Space",
	"Capital",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",
	"F9",
	"F10",
	"NumLock",
	"ScrollLock",
	"Numpad7",
	"Numpad8",
	"Numpad9",
	"Subtract",
	"Numpad4",
	"Numpad5",
	"Numpad6",
	"Add",
	"Numpad1",
	"Numpad2",
	"Numpad3",
	"Numpad0",
	"Decimal",
	"OEM_102",
	"F11",
	"F12",
	"F13",
	"F14",
	"F15",
	"Kana",
	"ABNT_C1",
	"Convert",
	"NoConvert",
	"Yen",
	"ABNT_C2",
	"NumpadEquals",
	"PrevTrack",
	"At",
	"Colon",
	"Underline",
	"Kanji",
	"Stop",
	"AX",
	"Unlabeled",
	"NextTrack",
	"NumpadEnter",
	"RightControl",
	"Mute",
	"Calculator",
	"PlayPause",
	"MediaStop",
	"VolumeDown",
	"VolumeUp",
	"WebHome",
	"NumpadComma",
	"Divide",
	"SysRq",
	"RightAlt8",
	"Pause",
	"Home",
	"ArrowUp",
	"PageUp",
	"ArrowLeft",
	"ArrowRight",
	"End",
	"ArrowDown",
	"PageDown",
	"Insert",
	"Delete",
	"LeftWindows",
	"RightWindows",
	"AppMenu",
	"Power",
	"Sleep",
	"Wake",
	"WebSearch",
	"WebFavorites",
	"WebRefresh",
	"WebStop",
	"WebForward",
	"WebBack",
	"MyComputer",
	"Mail",
	"MediaSelect"
};

const char*	DeviceTypeToString(EDeviceType Type)
{
	if (Type >= Dev_Count) return NULL;
	return pDeviceTypeString[Type];
}
//---------------------------------------------------------------------

EDeviceType	StringToDeviceType(const char* pName)
{
	for (UPTR i = 0; i < Dev_Count; ++i)
		if (!n_stricmp(pName, pDeviceTypeString[i]))
			return (EDeviceType)i;
	if (!n_stricmp(pName, "kb")) return Dev_Keyboard;
	if (!n_stricmp(pName, "xbox")) return Dev_Gamepad;
	return Dev_Invalid;
}
//---------------------------------------------------------------------

const char*	MouseAxisToString(EMouseAxis Axis)
{
	if (Axis >= MA_Count) return NULL;
	return pMouseAxisString[Axis];
}
//---------------------------------------------------------------------

EMouseAxis StringToMouseAxis(const char* pName)
{
	for (UPTR i = 0; i < MA_Count; ++i)
		if (!n_stricmp(pName, pMouseAxisString[i]))
			return (EMouseAxis)i;
	return MA_Invalid;
}
//---------------------------------------------------------------------

const char*	MouseButtonToString(EMouseButton Button)
{
	if (Button > MB_User)
	{
		//UPTR UserIndex = Button - MB_User;
		//if (UserIndex > 99) return NULL;
		//char Buffer[]
		//will return ptr to local, implement through external buffer if needed at all
		return NULL;
	}
	return pMouseButtonString[Button];
}
//---------------------------------------------------------------------

EMouseButton StringToMouseButton(const char* pName)
{
	for (UPTR i = 0; i <= MB_User; ++i)
		if (!n_stricmp(pName, pMouseButtonString[i]))
			return (EMouseButton)i;
	return MB_Invalid;
}
//---------------------------------------------------------------------

const char* KeyToString(EKey Key)
{
	if (Key >= Key_Count) return NULL;
	return pKeyString[Key];
}
//---------------------------------------------------------------------

EKey StringToKey(const char* pName)
{
	for (UPTR i = 0; i < Key_Count; ++i)
		if (!n_stricmp(pName, pKeyString[i]))
			return (EKey)i;
	if (!n_stricmp(pName, "~") || !n_stricmp(pName, "`")) return Grave;
	return Key_Invalid;
}
//---------------------------------------------------------------------

U8 StringToButton(EDeviceType DeviceType, const char* pName)
{
	switch (DeviceType)
	{
		case Dev_Keyboard:	return StringToKey(pName);
		case Dev_Mouse:		return StringToMouseButton(pName);
	}
	return InvalidCode;
}
//---------------------------------------------------------------------

}