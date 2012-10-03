#include "Subscription.h"

#include "EventDispatcher.h"

namespace Events
{

void CSubscription::Unsubscribe()
{
#ifdef _DEBUG
	n_assert(Dispatcher);
#endif

	Dispatcher->Unsubscribe(Event, Handler);

#ifdef _DEBUG
	Dispatcher = NULL;
#endif
}
//---------------------------------------------------------------------

}
