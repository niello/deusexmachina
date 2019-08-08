#include "SystemEvents.h"

namespace Event
{
__ImplementClassNoFactory(Event::OSWindowResized, Events::CEventNative);
__ImplementClassNoFactory(Event::InputDeviceArrived, Events::CEventNative);
__ImplementClassNoFactory(Event::InputDeviceRemoved, Events::CEventNative);
}