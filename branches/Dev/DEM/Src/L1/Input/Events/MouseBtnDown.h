#pragma once
#ifndef __DEM_L1_EVENT_MOUSE_BTN_DOWN_H__
#define __DEM_L1_EVENT_MOUSE_BTN_DOWN_H__

#include <Events/EventNative.h>
#include <Input/Keys.h>

// Input event

namespace Event
{

//???inherit from InputEvent where DeviceID is stored?
class MouseBtnDown: public Events::CEventNative
{
	DeclareRTTI;

public:

	Input::EMouseButton	Button;
	int					X, Y;
	float				XRel, YRel;

	MouseBtnDown() {}
	MouseBtnDown(Input::EMouseButton Btn, int x, int y, float xrel, float yrel): Button(Btn), X(x), Y(y), XRel(xrel), YRel(yrel) {}
};

}

#endif
