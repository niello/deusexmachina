#pragma once
#ifndef __DEM_L1_EVENT_MOUSE_MOVE_H__
#define __DEM_L1_EVENT_MOUSE_MOVE_H__

#include <Events/EventNative.h>

// Event is fired on mouse cursor movement.
// Stores new cursor position in pixels and normalized pos relative to upper-left corner of the display.

namespace Event
{

//???inherit from InputEvent where DeviceID is stored?
class MouseMove: public Events::CEventNative
{
	__DeclareNativeEventClass;

public:

	int		X, Y;
	float	XRel, YRel;

	MouseMove() {}
	MouseMove(int x, int y, float xrel, float yrel): X(x), Y(y), XRel(xrel), YRel(yrel) {}
};

}

#endif
