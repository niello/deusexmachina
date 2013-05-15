#pragma once
#ifndef __DEM_L1_EVENT_MOUSE_BTN_UP_H__
#define __DEM_L1_EVENT_MOUSE_BTN_UP_H__

#include <Events/EventNative.h>
#include <Input/Keys.h>

// Input event

namespace Event
{

//???inherit from InputEvent where DeviceID is stored?
class MouseBtnUp: public Events::CEventNative
{
	__DeclareClassNoFactory;

public:

	Input::EMouseButton	Button;
	int					X, Y;
	float				XRel, YRel;

	MouseBtnUp() {}
	MouseBtnUp(Input::EMouseButton Btn, int x, int y, float xrel, float yrel): Button(Btn), X(x), Y(y), XRel(xrel), YRel(yrel) {}
};

}

#endif
