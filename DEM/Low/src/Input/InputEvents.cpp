#include "InputEvents.h"

namespace Event
{
__ImplementClassNoFactory(Event::AxisMove, Events::CEventNative);
__ImplementClassNoFactory(Event::ButtonDown, Events::CEventNative);
__ImplementClassNoFactory(Event::ButtonUp, Events::CEventNative);
__ImplementClassNoFactory(Event::TextInput, Events::CEventNative);
}