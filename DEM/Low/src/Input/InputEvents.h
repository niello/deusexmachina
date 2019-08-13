#pragma once
#include <Events/EventNative.h>
#include <Input/InputDevice.h>
#include <string>

// Input device axis and button events

namespace Event
{

class AxisMove: public Events::CEventNative
{
	__DeclareNativeEventClass;

public:

	Input::PInputDevice	Device;
	CStrID				UserID;
	U8					Code;
	float				Amount;

	AxisMove(Input::PInputDevice Source, U8 AxisCode, float MoveAmount, CStrID User = CStrID::Empty)
		: Device(Source)
		, UserID(User)
		, Code(AxisCode)
		, Amount(MoveAmount)
	{}
};

class ButtonDown: public Events::CEventNative
{
	__DeclareNativeEventClass;

public:

	Input::PInputDevice	Device;
	CStrID				UserID;
	U8					Code;

	ButtonDown(Input::PInputDevice Source, U8 ButtonCode, CStrID User = CStrID::Empty)
		: Device(Source)
		, UserID(User)
		, Code(ButtonCode)
	{}
};

class ButtonUp: public Events::CEventNative
{
	__DeclareNativeEventClass;

public:

	Input::PInputDevice	Device;
	CStrID				UserID;
	U8					Code;

	ButtonUp(Input::PInputDevice Source, U8 ButtonCode, CStrID User = CStrID::Empty)
		: Device(Source)
		, UserID(User)
		, Code(ButtonCode)
	{}
};

class TextInput: public Events::CEventNative
{
	__DeclareNativeEventClass;

public:

	Input::PInputDevice	Device;
	CStrID				UserID;
	std::string			Text;
	bool				CaseSensitive = false;

	TextInput(Input::PInputDevice Source, std::string&& TextData, bool _CaseSensitive, CStrID User = CStrID::Empty)
		: Device(Source)
		, UserID(User)
		, Text(std::move(TextData))
		, CaseSensitive(_CaseSensitive)
	{}

	TextInput(Input::PInputDevice Source, const std::string& TextData, bool _CaseSensitive, CStrID User = CStrID::Empty)
		: Device(Source)
		, UserID(User)
		, Text(TextData)
		, CaseSensitive(_CaseSensitive)
	{}
};

}
