#pragma once
#include <StdDEM.h>

// Input system forward declarations

namespace Core
{
	class CRTTIBaseClass;
}

namespace Input
{
class IInputDevice;

constexpr U8 InvalidCode = 0xff;

// Input device types supported by an engine

enum EDeviceType
{
	Device_Keyboard,
	Device_Mouse,
	Device_Gamepad,
	Device_SpeechRecognition,

	Device_Count,
	Device_Invalid
};

// Typical mouse axes with corresponding codes and name text IDs.
// Provided for consistency between different mouse implementations,
// but your IInputDevice-based implementation may use other values
// for both codes and names.

enum EMouseAxis
{
	MouseAxis_X			= 0,
	MouseAxis_Y			= 1,
	MouseAxis_Wheel1	= 2,
	MouseAxis_Wheel2	= 3,

	MouseAxis_Count,
	MouseAxis_Invalid	= InvalidCode
};

const char*	MouseAxisToString(EMouseAxis Axis);
EMouseAxis	StringToMouseAxis(const char* pName);

// Typical mouse buttons with corresponding codes and name text IDs.
// See EMouseAxis comment above, the same applies here.

enum EMouseButton
{
	MouseButton_Left	= 0,
	MouseButton_Right	= 1,
	MouseButton_Middle	= 2,
	MouseButton_X1		= 3,
	MouseButton_X2		= 4,

	MouseButton_Count,
	MouseButton_Invalid	= InvalidCode
};

const char*		MouseButtonToString(EMouseButton Button);
EMouseButton	StringToMouseButton(const char* pName);

// Typical keyboard keys with corresponding codes and name text IDs.
// Values are based on standard scan codes, but some mapping may still be needed in a device code.
// List is taken from CEGUI <- Ogre <- DirectInput, some names changed for unambiguity and evidence.
enum EKey
{
	Escape          =0x01,
	One             =0x02,
	Two             =0x03,
	Three           =0x04,
	Four            =0x05,
	Five            =0x06,
	Six             =0x07,
	Seven           =0x08,
	Eight           =0x09,
	Nine            =0x0A,
	Zero            =0x0B,
	Minus           =0x0C,    /* - on main keyboard */
	Equals			=0x0D,
	Backspace		=0x0E,
	Tab				=0x0F,
	Q               =0x10,
	W               =0x11,
	E               =0x12,
	R               =0x13,
	T               =0x14,
	Y               =0x15,
	U               =0x16,
	I               =0x17,
	O               =0x18,
	P               =0x19,
	LeftBracket     =0x1A,
	RightBracket    =0x1B,
	Enter			=0x1C,    /* Enter on main keyboard */
	LeftControl		=0x1D,
	A               =0x1E,
	S               =0x1F,
	D               =0x20,
	F               =0x21,
	G               =0x22,
	H               =0x23,
	J               =0x24,
	K               =0x25,
	L               =0x26,
	Semicolon       =0x27,
	Apostrophe		=0x28,
	Grave           =0x29,    /* accent grave */
	LeftShift       =0x2A,
	Backslash       =0x2B,
	Z               =0x2C,
	X               =0x2D,
	C               =0x2E,
	V               =0x2F,
	B               =0x30,
	N               =0x31,
	M               =0x32,
	Comma           =0x33,
	Period          =0x34,    /* . on main keyboard */
	Slash           =0x35,    /* '/' on main keyboard */
	RightShift      =0x36,
	NumpadMul       =0x37,    /* * on numeric keypad */
	LeftAlt         =0x38,
	Space           =0x39,
	CapsLock        =0x3A,
	F1              =0x3B,
	F2              =0x3C,
	F3              =0x3D,
	F4              =0x3E,
	F5              =0x3F,
	F6              =0x40,
	F7              =0x41,
	F8              =0x42,
	F9              =0x43,
	F10             =0x44,
	NumLock         =0x45,
	ScrollLock      =0x46,
	Numpad7         =0x47,
	Numpad8         =0x48,
	Numpad9         =0x49,
	NumpadMinus     =0x4A,    /* - on numeric keypad */
	Numpad4         =0x4B,
	Numpad5         =0x4C,
	Numpad6         =0x4D,
	NumpadPlus		=0x4E,    /* + on numeric keypad */
	Numpad1         =0x4F,
	Numpad2         =0x50,
	Numpad3         =0x51,
	Numpad0         =0x52,
	Decimal			=0x53,    /* . on numeric keypad */
	OEM_102         =0x56,    /* < > | on UK/Germany keyboards */
	F11             =0x57,
	F12             =0x58,
	F13             =0x64,    /*                     (NEC PC98) */
	F14             =0x65,    /*                     (NEC PC98) */
	F15             =0x66,    /*                     (NEC PC98) */
	Kana            =0x70,    /* (Japanese keyboard)            */
	ABNT_C1         =0x73,    /* / ? on Portugese (Brazilian) keyboards */
	Convert         =0x79,    /* (Japanese keyboard)            */
	NoConvert       =0x7B,    /* (Japanese keyboard)            */
	Yen             =0x7D,    /* (Japanese keyboard)            */
	ABNT_C2         =0x7E,    /* Numpad . on Portugese (Brazilian) keyboards */
	NumpadEquals    =0x8D,    /* = on numeric keypad (NEC PC98) */
	PrevTrack       =0x90,    /* Previous Track (KC_CIRCUMFLEX on Japanese keyboard) */
	At              =0x91,    /*                     (NEC PC98) */
	Colon           =0x92,    /*                     (NEC PC98) */
	Underline       =0x93,    /*                     (NEC PC98) */
	Kanji           =0x94,    /* (Japanese keyboard)            */
	Stop            =0x95,    /*                     (NEC PC98) */
	AX              =0x96,    /*                     (Japan AX) */
	Unlabeled       =0x97,    /*                        (J3100) */
	NextTrack       =0x99,    /* Next Track */
	NumpadEnter     =0x9C,    /* Enter on numeric keypad */
	RightControl    =0x9D,
	Mute            =0xA0,    /* Mute */
	Calculator      =0xA1,    /* Calculator */
	PlayPause       =0xA2,    /* Play / Pause */
	MediaStop       =0xA4,    /* Media Stop */
	VolumeDown      =0xAE,    /* Volume - */
	VolumeUp        =0xB0,    /* Volume + */
	WebHome         =0xB2,    /* Web home */
	NumpadComma     =0xB3,    /* , on numeric keypad (NEC PC98) */
	Divide          =0xB5,    /* / on numeric keypad */
	PrintScreen     =0xB7,	  /* Print Screen / Sys Rq */
	RightAlt        =0xB8,    /* right Alt */
	Pause           =0xC5,    /* Pause */
	Home            =0xC7,    /* Home on arrow keypad */
	ArrowUp         =0xC8,    /* UpArrow on arrow keypad */
	PageUp          =0xC9,    /* PgUp on arrow keypad */
	ArrowLeft       =0xCB,    /* LeftArrow on arrow keypad */
	ArrowRight      =0xCD,    /* RightArrow on arrow keypad */
	End             =0xCF,    /* End on arrow keypad */
	ArrowDown       =0xD0,    /* DownArrow on arrow keypad */
	PageDown		=0xD1,    /* PgDn on arrow keypad */
	Insert          =0xD2,    /* Insert on arrow keypad */
	Delete          =0xD3,    /* Delete on arrow keypad */
	LeftWindows     =0xDB,    /* Left Windows key */
	RightWindows    =0xDC,    /* Right Windows key - Correct spelling :) */
	AppMenu         =0xDD,    /* AppMenu key */
	Power           =0xDE,    /* System Power */
	Sleep           =0xDF,    /* System Sleep */
	Wake			=0xE3,    /* System Wake */
	WebSearch		=0xE5,    /* Web Search */
	WebFavorites	=0xE6,    /* Web Favorites */
	WebRefresh		=0xE7,    /* Web Refresh */
	WebStop			=0xE8,    /* Web Stop */
	WebForward		=0xE9,    /* Web Forward */
	WebBack			=0xEA,    /* Web Back */
	MyComputer		=0xEB,    /* My Computer */
	Mail			=0xEC,    /* Mail */
	MediaSelect		=0xED,    /* Media Select */

	Key_Count		= 144,			// Real key count
	Key_Invalid		= InvalidCode
};

const char*				KeyToString(EKey Key);
EKey					StringToKey(const char* pName);

U8						StringToButton(EDeviceType DeviceType, const char* pName);
const char*				ButtonToString(EDeviceType DeviceType, U8 Code);

Core::CRTTIBaseClass*	ParseRule(const char* pRule);

}

namespace Event
{
	class AxisMove;
	class ButtonDown;
	class ButtonUp;
	class TextInput;
}
