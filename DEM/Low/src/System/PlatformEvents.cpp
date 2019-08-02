#include "PlatformEvents.h"

namespace Event
{
__ImplementClassNoFactory(Event::InputDeviceArrived, Events::CEventNative);
__ImplementClassNoFactory(Event::InputDeviceRemoved, Events::CEventNative);
}