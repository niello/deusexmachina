#pragma once
#ifndef __DEM_L1_EVENT_MOUSE_MOVE_RAW_H__
#define __DEM_L1_EVENT_MOUSE_MOVE_RAW_H__

#include <Events/EventNative.h>

// Event is fired on mouse movement (NOT cursor!).
// Stores movement in device units since the previous MouseMoveRaw.

namespace Event
{

//???inherit from InputEvent where DeviceID is stored?
class MouseMoveRaw: public Events::CEventNative
{
	__DeclareClassNoFactory;

public:

	int X, Y;

	MouseMoveRaw(int x, int y): X(x), Y(y) {}
};

}

#endif
