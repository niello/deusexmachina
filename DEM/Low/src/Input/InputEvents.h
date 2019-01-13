#pragma once
#ifndef __DEM_L1_INPUT_EVENTS_H__
#define __DEM_L1_INPUT_EVENTS_H__

#include <Events/EventNative.h>

// IInputDevice axis and button events

namespace Event
{

class AxisMove: public Events::CEventNative
{
	__DeclareNativeEventClass;

public:

	U8		Code;
	float	Amount; //???I16/I32?

	AxisMove(U8 AxisCode, float MoveAmount): Code(AxisCode), Amount(MoveAmount) {}
};

class ButtonDown: public Events::CEventNative
{
	__DeclareNativeEventClass;

public:

	U8 Code;

	ButtonDown(U8 ButtonCode): Code(ButtonCode) {}
};

class ButtonUp: public Events::CEventNative
{
	__DeclareNativeEventClass;

public:

	U8 Code;

	ButtonUp(U8 ButtonCode): Code(ButtonCode) {}
};

}

#endif
