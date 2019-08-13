#include "InputFwd.h"
#include <string.h>

namespace Input
{

// Human-readable name (localization ID)
static const char* pDeviceTypeString[] =
{
	"ID_Input_Keyboard",
	"ID_Input_Mouse",
	"ID_Input_Gamepad",
	"ID_Input_SpeechRecognition",
};

// Internal ID
static const char* pMouseAxisString[] =
{
	"AxisX",
	"AxisY",
	"Wheel",
	"HWheel"
};

// Internal ID
static const char* pMouseButtonString[] =
{
	"LMB",
	"RMB",
	"MMB",
	"X1",
	"X2"
};

// Internal ID
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
	"Enter",
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
	"NumpadMul",
	"LeftAlt",
	"Space",
	"CapsLock",
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
	"NumpadMinus",
	"Numpad4",
	"Numpad5",
	"Numpad6",
	"NumpadPlus",
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
	"PrintScreen",
	"RightAlt",
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

static U8 pKeyCode[] =
{
	Escape,
	One,
	Two,
	Three,
	Four,
	Five,
	Six,
	Seven,
	Eight,
	Nine,
	Zero,
	Minus,
	Equals,
	Backspace,
	Tab,
	Q,
	W,
	E,
	R,
	T,
	Y,
	U,
	I,
	O,
	P,
	LeftBracket,
	RightBracket,
	Enter,
	LeftControl,
	A,
	S,
	D,
	F,
	G,
	H,
	J,
	K,
	L,
	Semicolon,
	Apostrophe,
	Grave,
	LeftShift,
	Backslash,
	Z,
	X,
	C,
	V,
	B,
	N,
	M,
	Comma,
	Period,
	Slash,
	RightShift,
	NumpadMul,
	LeftAlt,
	Space,
	Capital,
	F1,
	F2,
	F3,
	F4,
	F5,
	F6,
	F7,
	F8,
	F9,
	F10,
	NumLock,
	ScrollLock,
	Numpad7,
	Numpad8,
	Numpad9,
	NumpadMinus,
	Numpad4,
	Numpad5,
	Numpad6,
	NumpadPlus,
	Numpad1,
	Numpad2,
	Numpad3,
	Numpad0,
	Decimal,
	OEM_102,
	F11,
	F12,
	F13,
	F14,
	F15,
	Kana,
	ABNT_C1,
	Convert,
	NoConvert,
	Yen,
	ABNT_C2,
	NumpadEquals,
	PrevTrack,
	At,
	Colon,
	Underline,
	Kanji,
	Stop,
	AX,
	Unlabeled,
	NextTrack,
	NumpadEnter,
	RightControl,
	Mute,
	Calculator,
	PlayPause,
	MediaStop,
	VolumeDown,
	VolumeUp,
	WebHome,
	NumpadComma,
	Divide,
	PrintScreen,
	RightAlt,
	Pause,
	Home,
	ArrowUp,
	PageUp,
	ArrowLeft,
	ArrowRight,
	End,
	ArrowDown,
	PageDown,
	Insert,
	Delete,
	LeftWindows,
	RightWindows,
	AppMenu,
	Power,
	Sleep,
	Wake,
	WebSearch,
	WebFavorites,
	WebRefresh,
	WebStop,
	WebForward,
	WebBack,
	MyComputer,
	Mail,
	MediaSelect
};

const char*	MouseAxisToString(EMouseAxis Axis)
{
	if (Axis >= MouseAxis_Count) return nullptr;
	return pMouseAxisString[Axis];
}
//---------------------------------------------------------------------

EMouseAxis StringToMouseAxis(const char* pName)
{
	for (UPTR i = 0; i < MouseAxis_Count; ++i)
		if (!n_stricmp(pName, pMouseAxisString[i]))
			return (EMouseAxis)i;
	return MouseAxis_Invalid;
}
//---------------------------------------------------------------------

const char*	MouseButtonToString(EMouseButton Button)
{
	if (Button >= MouseButton_Count) return nullptr;
	return pMouseButtonString[Button];
}
//---------------------------------------------------------------------

EMouseButton StringToMouseButton(const char* pName)
{
	for (UPTR i = 0; i < MouseButton_Count; ++i)
		if (!n_stricmp(pName, pMouseButtonString[i]))
			return (EMouseButton)i;
	return MouseButton_Invalid;
}
//---------------------------------------------------------------------

const char* KeyToString(EKey Key)
{
	for (UPTR i = 0; i < Key_Count; ++i)
		if (Key == pKeyCode[i])
			return pKeyString[i];
	return nullptr;
}
//---------------------------------------------------------------------

EKey StringToKey(const char* pName)
{
	for (UPTR i = 0; i < Key_Count; ++i)
		if (!n_stricmp(pName, pKeyString[i]))
			return (EKey)pKeyCode[i];
	if (!n_stricmp(pName, "~") || !n_stricmp(pName, "`")) return Grave;
	if (!n_stricmp(pName, "shift")) return LeftShift;
	if (!n_stricmp(pName, "ctrl")) return LeftControl;
	if (!n_stricmp(pName, "alt")) return LeftAlt;
	return Key_Invalid;
}
//---------------------------------------------------------------------

U8 StringToButton(EDeviceType DeviceType, const char* pName)
{
	switch (DeviceType)
	{
		case Device_Keyboard:	return StringToKey(pName);
		case Device_Mouse:		return StringToMouseButton(pName);
	}
	return InvalidCode;
}
//---------------------------------------------------------------------

}