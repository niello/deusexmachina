#pragma once
#include <Events/EventNative.h>
#include <Input/InputDevice.h>

// Platform events

namespace Event
{

class OSWindowResized: public Events::CEventNative
{
	NATIVE_EVENT_DECL(Event::OSWindowResized, Events::CEventNative);

public:

	U16 OldWidth;
	U16 OldHeight;
	U16 NewWidth;
	U16 NewHeight;
	bool ManualResizingInProgress;

	OSWindowResized(U16 _OldWidth, U16 _OldHeight, U16 _NewWidth, U16 _NewHeight, bool _ManualResizingInProgress)
		: OldWidth(_OldWidth)
		, OldHeight(_OldHeight)
		, NewWidth(_NewWidth)
		, NewHeight(_NewHeight)
		, ManualResizingInProgress(_ManualResizingInProgress)
	{}
};

class InputDeviceArrived: public Events::CEventNative
{
	NATIVE_EVENT_DECL(Event::InputDeviceArrived, Events::CEventNative);

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
	NATIVE_EVENT_DECL(Event::InputDeviceRemoved, Events::CEventNative);

public:

	Input::PInputDevice	Device;

	InputDeviceRemoved(Input::PInputDevice _Device)
		: Device(_Device)
	{}
};

}
