#include "Subscription.h"
#include <Events/EventDispatcher.h>

namespace Events
{

CSubscription::~CSubscription()
{
	if (pDispatcher) pDispatcher->Unsubscribe(Event, pHandler);
}
//---------------------------------------------------------------------

void CSubscription::Unsubscribe()
{
	pDispatcher->Unsubscribe(Event, pHandler);
	pDispatcher = nullptr;
	pHandler = nullptr;
}
//---------------------------------------------------------------------

}
