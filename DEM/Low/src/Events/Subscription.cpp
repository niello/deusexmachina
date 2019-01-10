#include "Subscription.h"
#include <Events/EventDispatcher.h>

namespace Events
{

CSubscription::CSubscription(CEventDispatcher* d, CEventID e, PEventHandler h):
	pDispatcher(d), Event(e), Handler(h)
{
}
//---------------------------------------------------------------------

CSubscription::~CSubscription()
{
	pDispatcher->Unsubscribe(Event, Handler);
}
//---------------------------------------------------------------------

}
