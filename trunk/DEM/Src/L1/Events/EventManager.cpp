#include "EventManager.h"

#include "EventDispatcher.h"

namespace Events
{
ImplementRTTI(Events::CEventManager, Core::CRefCounted);
__ImplementSingleton(Events::CEventManager);

CEventManager::CEventManager(): CEventDispatcher(256)
{
	__ConstructSingleton;
}
//---------------------------------------------------------------------

CEventManager::~CEventManager()
{
	//!!!FIXME!
	//!!!duplicate code from CEventDispatcher destructor!
	//cause pool destructs before this executes
	while (PendingEventsHead)
	{
		CEventNode* Next = PendingEventsHead->Next;
		EventMgr->EventNodes.Destroy(PendingEventsHead);
		PendingEventsHead = Next;
	}

	while (EventsToAdd)
	{
		CEventNode* Next = EventsToAdd->Next;
		EventMgr->EventNodes.Destroy(EventsToAdd);
		EventsToAdd = Next;
	}

	__DestructSingleton;
}
//---------------------------------------------------------------------

} //namespace AI