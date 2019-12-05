#include "SystemEvents.h"

namespace Event
{
RTTI_CLASS_IMPL(Event::OSWindowResized, Events::CEventNative);
RTTI_CLASS_IMPL(Event::InputDeviceArrived, Events::CEventNative);
RTTI_CLASS_IMPL(Event::InputDeviceRemoved, Events::CEventNative);
}