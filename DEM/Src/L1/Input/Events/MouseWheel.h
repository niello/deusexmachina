#pragma once
#ifndef __DEM_L1_EVENT_MOUSE_WHEEL_H__
#define __DEM_L1_EVENT_MOUSE_WHEEL_H__

#include <Events/EventNative.h>

// Input event

namespace Event
{

//???inherit from InputEvent where DeviceID is stored?
class MouseWheel: public Events::CEventNative
{
	__DeclareClassNoFactory;

public:

	int Delta;

	MouseWheel(int _Delta): Delta(_Delta) {}
};

}

#endif
