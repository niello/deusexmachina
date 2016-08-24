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
	float	AmountRel;
	float	AmountAbs;

	AxisMove(U8 AxisCode, float AmountRelative, float AmountAbsolute): Code(AxisCode), AmountRel(AmountRelative), AmountAbs(AmountAbsolute) {}
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
