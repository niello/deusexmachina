#include "InputFwd.h"
#include <Input/InputConditionText.h>
#include <Input/InputConditionPressed.h>
#include <Input/InputConditionReleased.h>
#include <Input/InputConditionHold.h>
#include <Input/InputConditionMove.h>
#include <Input/InputConditionUp.h>
#include <Input/InputConditionSequence.h>
#include <Input/InputConditionComboState.h>
#include <Input/InputConditionComboEvent.h>
#include <Input/InputConditionAnyOfEvents.h>
#include <Input/InputConditionAnyOfStates.h>
#include <Input/InputConditionEventTemplate.h>
#include <Input/InputConditionStateTemplate.h>
#include <string.h>
#include <cctype>

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

static Core::CRTTIBaseClass* ParseCondition(const char*& pRule)
{
	const char* pCurr = pRule;

	// Skip leading whitespace
	while (std::isspace(*pCurr)) ++pCurr;

	const char* pRuleStart = pCurr;

	char c = *pCurr;

	// Rule is empty, it is an error
	if (!c || c == '|') return nullptr;

	// Text condition
	if (c == '\'')
	{
		++pCurr;
		const char* pEnd = strchr(pCurr, '\'');
		if (!pEnd) return nullptr;
		pRule = pEnd + 1;
		return new CInputConditionText(std::string(pCurr, pEnd));
	}

	// Axis or button condition
	// TODO: can use std::isprint to parse key symbols like "~"
	if (c == '$' || c == '_' || std::isalnum(static_cast<unsigned char>(c)))
	{
		const bool IsParam = (c == '$');
		if (IsParam) ++pCurr;

		const char* pEnd = pCurr;
		c = *pEnd;
		while (c == '_' || std::isalnum(static_cast<unsigned char>(c)))
		{
			++pEnd;
			c = *pEnd;
		}

		std::string ID(pCurr, pEnd);

		EDeviceType DeviceType = Device_Invalid;
		bool IsAxis = false;
		U8 Code = InvalidCode;

		if (!IsParam)
		{
			// Resolve input channel from ID
			Code = static_cast<U8>(StringToKey(ID.c_str()));
			if (Code != InvalidCode) DeviceType = Device_Keyboard;
			else
			{
				Code = static_cast<U8>(StringToMouseButton(ID.c_str()));
				if (Code != InvalidCode) DeviceType = Device_Mouse;
				else
				{
					Code = static_cast<U8>(StringToMouseAxis(ID.c_str()));
					if (Code != InvalidCode)
					{
						DeviceType = Device_Mouse;
						IsAxis = true;
					}
					else return nullptr; // TODO: gamepad
				}
			}
		}

		// Process optional modifiers
		switch (c)
		{
			case '+':
			{
				if (IsAxis) return nullptr;
				pRule = pEnd + 1;
				if (IsParam)
					return new CInputConditionStateTemplate(std::string(pRuleStart, pEnd + 1));
				else
					return new CInputConditionPressed(DeviceType, Code);
			}
			case '-':
			{
				if (IsAxis) return nullptr;
				pRule = pEnd + 1;
				if (IsParam)
					return new CInputConditionStateTemplate(std::string(pRuleStart, pEnd + 1));
				else
					return new CInputConditionReleased(DeviceType, Code);
			}
			case '[':
			case '{':
			{
				const bool Repeat = (c == '{');
				pCurr = pEnd + 1;
				pEnd = strchr(pCurr, Repeat ? '}' : ']');
				if (!pEnd) return nullptr;

				char* pParseEnd = nullptr;
				const float TimeOrAmount = strtof(pCurr, &pParseEnd);
				if (pParseEnd != pEnd) return nullptr;

				pRule = pEnd + 1;

				if (IsParam)
					return new CInputConditionEventTemplate(std::string(pRuleStart, pEnd + 1));
				else if (IsAxis)
					return new CInputConditionMove(DeviceType, Code, TimeOrAmount);
				else
					return new CInputConditionHold(DeviceType, Code, TimeOrAmount, Repeat);
			}
			default:
			{
				pRule = pEnd;

				if (IsParam)
					return new CInputConditionEventTemplate(std::string(pRuleStart, pEnd));
				else if (IsAxis)
					return new CInputConditionMove(DeviceType, Code);
				else
					return new CInputConditionUp(DeviceType, Code);
			}
		}
	}

	// Unknown condition type
	return nullptr;
}
//---------------------------------------------------------------------

static Core::CRTTIBaseClass* ParseSequence(const char*& pRule)
{
	Core::CRTTIBaseClass* pResult = nullptr;

	while (*pRule && *pRule != '|')
	{
		Core::CRTTIBaseClass* pCondition = ParseCondition(pRule);

		// Failed parsing is an error, result is discarded
		if (!pCondition)
		{
			n_delete(pResult);
			return nullptr;
		}

		if (!pResult) pResult = pCondition;
		else
		{
			const bool FirstIsEvent = pResult->IsA<CInputConditionEvent>();
			const bool SecondIsEvent = pCondition->IsA<CInputConditionEvent>();
			if (FirstIsEvent && SecondIsEvent)
			{
				// Events are combined into sequences
				auto pSequence = pResult->As<CInputConditionSequence>();
				if (!pSequence)
				{
					pSequence = new CInputConditionSequence();
					pSequence->AddChild(PInputConditionEvent(static_cast<CInputConditionEvent*>(pResult)));
					pResult = pSequence;
				}
				pSequence->AddChild(PInputConditionEvent(static_cast<CInputConditionEvent*>(pCondition)));
			}
			else if (!FirstIsEvent && !SecondIsEvent)
			{
				// States are combined into one combo state
				auto pCombo = pResult->As<CInputConditionComboState>();
				if (!pCombo)
				{
					pCombo = new CInputConditionComboState();
					pCombo->AddChild(PInputConditionState(static_cast<CInputConditionState*>(pResult)));
					pResult = pCombo;
				}
				pCombo->AddChild(PInputConditionState(static_cast<CInputConditionState*>(pCondition)));
			}
			else
			{
				// Events and states are mixed through the combo event. Once it is created, all events
				// are combined into one sequence and all states into one combo state inside a combo event.

				auto pEvent = FirstIsEvent ? pResult : pCondition;
				auto pState = FirstIsEvent ? pCondition : pResult;

				auto pCombo = pEvent->As<CInputConditionComboEvent>();
				if (!pCombo)
				{
					pCombo = new CInputConditionComboEvent();
					pCombo->AddChild(PInputConditionEvent(static_cast<CInputConditionEvent*>(pEvent)));
				}
				pCombo->AddChild(PInputConditionState(static_cast<CInputConditionState*>(pState)));

				pResult = pCombo;
			}
		}

		// Skip trailing whitespace
		while (std::isspace(*pRule)) ++pRule;
	}

	return pResult;
}
//---------------------------------------------------------------------

Core::CRTTIBaseClass* ParseRule(const char* pRule)
{
	Core::CRTTIBaseClass* pResult = nullptr;
	bool IsSequence = true;

	do
	{
		Core::CRTTIBaseClass* pCondition = ParseSequence(pRule);

		if (!pCondition)
		{
			// Failed parsing is an error, result is discarded
			n_delete(pResult);
			return nullptr;
		}

		if (!pResult) pResult = pCondition;
		else
		{
			const bool FirstIsEvent = pResult->IsA<CInputConditionEvent>();
			const bool SecondIsEvent = pCondition->IsA<CInputConditionEvent>();
			if (FirstIsEvent != SecondIsEvent)
			{
				// Can't mix events and states with OR
				n_delete(pResult);
				return nullptr;
			}

			if (FirstIsEvent)
			{
				auto pAny = pResult->As<CInputConditionAnyOfEvents>();
				if (!pAny)
				{
					pAny = new CInputConditionAnyOfEvents();
					pAny->AddChild(PInputConditionEvent(static_cast<CInputConditionEvent*>(pResult)));
					pResult = pAny;
				}
				pAny->AddChild(PInputConditionEvent(static_cast<CInputConditionEvent*>(pCondition)));
			}
			else
			{
				auto pAny = pResult->As<CInputConditionAnyOfStates>();
				if (!pAny)
				{
					pAny = new CInputConditionAnyOfStates();
					pAny->AddChild(PInputConditionState(static_cast<CInputConditionState*>(pResult)));
					pResult = pAny;
				}
				pAny->AddChild(PInputConditionState(static_cast<CInputConditionState*>(pCondition)));
			}
		}

		// Skip trailing whitespace
		while (std::isspace(*pRule)) ++pRule;
	}
	while (*pRule++ == '|');

	return pResult;
}
//---------------------------------------------------------------------

}