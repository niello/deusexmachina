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

public:

	enum EType
	{
		Invalid,
		KeyDown,
		KeyUp,
		MouseMoveRaw,	// Raw device movement
		MouseMove,		// Cursor movement //???need? how to process mouse in UI?
		MouseDown,
		MouseUp,
		MouseDblClick,
		MouseWheelVertical,
		MouseWheelHorizontal
	};

	enum EKeyFlags
	{
		Key_Repeated	= 0x01,
		Key_Extended	= 0x02
	};

	EType			Type;

	union
	{
		struct
		{
			U32			Char;		// UTF-16 or UTF-32 character
			U8			ScanCode;
			U8			VirtualKey;
			U8			Flags;
			//???need repeat count?
		}			KeyboardInfo;
		struct
		{
			IPTR		x;
			IPTR		y;
			U8			Button;
		}			MouseInfo;
		IPTR		WheelDelta;
	};
};

}

#endif
