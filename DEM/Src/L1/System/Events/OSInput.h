#pragma once
#ifndef __DEM_L1_EVENT_DISPLAY_INPUT_H__
#define __DEM_L1_EVENT_DISPLAY_INPUT_H__

#include <Events/EventNative.h>

// Fired when input is received by the display (application window)

namespace Event
{

class OSInput: public Events::CEventNative
{
	__DeclareNativeEventClass;

protected:

	struct CMouseInfo
	{
		IPTR	x;
		IPTR	y;
		U8		Button;
	};

public:

	enum EType
	{
		Invalid,
		KeyDown,
		KeyUp,
		CharInput,
		MouseMoveRaw,	// Raw device movement
		MouseMove,		// Cursor movement //???need? how to process mouse in UI?
		MouseDown,
		MouseUp,
		MouseDblClick,
		MouseWheelVertical,
		MouseWheelHorizontal
	};

	EType			Type;

	union
	{
		U8			KeyCode;	// Keyboard scan code
		U16			Char;		// UTF-16 character
		CMouseInfo	MouseInfo;
		IPTR		WheelDelta;
	};
};

}

#endif
