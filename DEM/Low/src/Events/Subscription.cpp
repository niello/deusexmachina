#include "Subscription.h"
#include <Events/EventDispatcher.h>

namespace Events
{

CSubscription::~CSubscription()
{
	pDispatcher->Unsubscribe(Event, pHandler);
}
//---------------------------------------------------------------------

}
