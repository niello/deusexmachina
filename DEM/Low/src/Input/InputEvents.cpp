#include "InputEvents.h"

namespace Event
{
RTTI_CLASS_IMPL(Event::AxisMove, Events::CEventNative);
RTTI_CLASS_IMPL(Event::ButtonDown, Events::CEventNative);
RTTI_CLASS_IMPL(Event::ButtonUp, Events::CEventNative);
RTTI_CLASS_IMPL(Event::TextInput, Events::CEventNative);
}