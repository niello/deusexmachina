#pragma once
#include <Events/EventNative.h>
#include <Input/InputDevice.h>

// Platform events

namespace Event
{

class InputDeviceArrived: public Events::CEventNative
{
	__DeclareNativeEventClass;

public:

	Input::PInputDevice	Device;
	bool				FirstSeen;

	InputDeviceArrived(Input::PInputDevice _Device, bool _FirstSeen)
		: Device(_Device)
		, FirstSeen(_FirstSeen)
	{}
};

class InputDeviceRemoved: public Events::CEventNative
{
	__DeclareNativeEventClass;

public:

	Input::PInputDevice	Device;

	InputDeviceRemoved(Input::PInputDevice _Device)
		: Device(_Device)
	{}
};

}
