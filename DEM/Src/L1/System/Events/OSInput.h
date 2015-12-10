#pragma once
#ifndef __DEM_L1_EVENT_DISPLAY_INPUT_H__
#define __DEM_L1_EVENT_DISPLAY_INPUT_H__

#include <Events/EventNative.h>
#include <Input/Keys.h>

// Fired when input is received by the display (application window)

namespace Event
{

class OSInput: public Events::CEventNative
{
	__DeclareNativeEventClass;

protected:

	struct CMouseInfo
	{
		int					x;
		int					y;
		Input::EMouseButton	Button;
	};

public:

	enum EType
	{
		Invalid,
		KeyDown,
		KeyUp,
		CharInput,
		MouseMoveRaw,	// Raw device movement
		MouseMove,		// Cursor movement
		MouseDown,
		MouseUp,
		MouseDblClick,
		MouseWheel
	};

	EType			Type;

	union
	{
		Input::EKey	KeyCode;	// Keyboard scan code
		U16		Char;		// UTF-16 character
		CMouseInfo	MouseInfo;
		int			WheelDelta;
	};
};

}

#endif
