#pragma once
#include <Events/EventNative.h>
#include <Input/InputDevice.h>
#include <string>

// IInputDevice axis and button events

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

	AxisMove(Input::PInputDevice Source, U8 AxisCode, float MoveAmount)
		: Device(Source)
		, Code(AxisCode)
		, Amount(MoveAmount)
	{
	}
};

class ButtonDown: public Events::CEventNative
{
	__DeclareNativeEventClass;

public:

	Input::PInputDevice	Device;
	CStrID				UserID;
	U8					Code;

	ButtonDown(Input::PInputDevice Source, U8 ButtonCode): Device(Source), Code(ButtonCode) {}
};

class ButtonUp: public Events::CEventNative
{
	__DeclareNativeEventClass;

public:

	Input::PInputDevice	Device;
	CStrID				UserID;
	U8					Code;

	ButtonUp(Input::PInputDevice Source, U8 ButtonCode): Device(Source), Code(ButtonCode) {}
};

class TextInput: public Events::CEventNative
{
	__DeclareNativeEventClass;

public:

	Input::PInputDevice	Device;
	std::string			Text;

	TextInput(Input::PInputDevice Source, std::string&& TextData): Device(Source), Text(std::move(TextData)) {}
	TextInput(Input::PInputDevice Source, const std::string& TextData): Device(Source), Text(TextData) {}
};

}
