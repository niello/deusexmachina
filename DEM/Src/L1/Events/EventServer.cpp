#include "EventServer.h"

#include "EventDispatcher.h"

namespace Events
{
__ImplementClassNoFactory(Events::CEventServer, Core::CRefCounted);
__ImplementSingleton(Events::CEventServer);

CEventServer::CEventServer(): CEventDispatcher(256)
{
	__ConstructSingleton;
}
//---------------------------------------------------------------------

CEventServer::~CEventServer()
{
	//!!!FIXME!
	//!!!duplicate code from CEventDispatcher destructor!
	//cause pool destructs before this executes
	while (PendingEventsHead)
	{
		CEventNode* Next = PendingEventsHead->Next;
		EventSrv->EventNodes.Destroy(PendingEventsHead);
		PendingEventsHead = Next;
	}

	while (EventsToAdd)
	{
		CEventNode* Next = EventsToAdd->Next;
		EventSrv->EventNodes.Destroy(EventsToAdd);
		EventsToAdd = Next;
	}

	__DestructSingleton;
}
//---------------------------------------------------------------------

} //namespace AI