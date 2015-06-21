#include "EventServer.h"

#include <Time/TimeServer.h>

namespace Events
{
__ImplementClassNoFactory(Events::CEventServer, Core::CObject);
__ImplementSingleton(Events::CEventServer);

void CEventServer::ScheduleEvent(CEventBase& Event, CEventDispatcher* pDisp, float RelTime)
{
	CEventNode* pNewNode = EventSrv->CreateNode();
	n_assert2(pNewNode, "Nervous system of the engine was paralyzed! Can't allocate event node");
	pNewNode->Event = &Event;
	pNewNode->Dispatcher = pDisp ? pDisp : this;
	pNewNode->FireTime = (float)TimeSrv->GetTime() + RelTime;
	if (PendingEventsTail)
	{
		n_assert(PendingEventsHead);

		if (pNewNode->FireTime >= PendingEventsTail->FireTime)
		{
			PendingEventsTail->Next = pNewNode;
			PendingEventsTail = pNewNode;
		}
		else
		{
			if (PendingEventsHead == PendingEventsTail)
			{
				PendingEventsHead = pNewNode;
				PendingEventsHead->Next = PendingEventsTail;
			}
			else if (EventsToAdd)
			{
				CEventNode* Curr = EventsToAdd;
				CEventNode* Next = EventsToAdd->Next;
				while (Curr)
				{
					if (pNewNode->FireTime >= Curr->FireTime)
					{
						Curr->Next = pNewNode;
						pNewNode->Next = Next;
						break;
					}
					Curr = Next;
					Next = Next->Next;
				}
			}
			else EventsToAdd = pNewNode;
		}
	}
	else
	{
		n_assert(!PendingEventsHead);
		PendingEventsHead = pNewNode;
		PendingEventsTail = pNewNode;
	}
}
//---------------------------------------------------------------------

DWORD CEventServer::RemoveScheduledEvents(CEventDispatcher* pDisp)
{
	CEventDispatcher* pRealDisp = pDisp ? pDisp : this;
	DWORD Total = 0;

	Sys::Error("CEventServer::RemoveScheduledEvents() > IMPLEMENT ME!!!");

	//while (PendingEventsHead)
	//{
	//	CEventNode* Next = PendingEventsHead->Next;
	//	EventSrv->DestroyNode(PendingEventsHead);
	//	PendingEventsHead = Next;
	//	++Total;
	//}

	//while (EventsToAdd)
	//{
	//	CEventNode* Next = EventsToAdd->Next;
	//	EventSrv->DestroyNode(EventsToAdd);
	//	EventsToAdd = Next;
	//	++Total;
	//}

	return Total;
}
//---------------------------------------------------------------------

DWORD CEventServer::RemoveAllScheduledEvents()
{
	DWORD Total = 0;

	while (PendingEventsHead)
	{
		CEventNode* Next = PendingEventsHead->Next;
		EventSrv->DestroyNode(PendingEventsHead);
		PendingEventsHead = Next;
		++Total;
	}

	while (EventsToAdd)
	{
		CEventNode* Next = EventsToAdd->Next;
		EventSrv->DestroyNode(EventsToAdd);
		EventsToAdd = Next;
		++Total;
	}

	return Total;
}
//---------------------------------------------------------------------

void CEventServer::ProcessPendingEvents()
{
	if (EventsToAdd)
	{
		n_assert(PendingEventsHead);

		CEventNode* Curr = PendingEventsHead;
		CEventNode* Next = PendingEventsHead->Next;
		while (Curr)
		{
			if (EventsToAdd->FireTime >= Curr->FireTime)
			{
				Curr->Next = EventsToAdd;
				while (EventsToAdd->Next && EventsToAdd->Next->FireTime <= Next->FireTime)
					EventsToAdd = EventsToAdd->Next;
				if (!EventsToAdd->Next)
				{
					EventsToAdd->Next = Next;
					EventsToAdd = NULL;
					break;
				}
				CEventNode* NextToAdd = EventsToAdd->Next;
				EventsToAdd->Next = Next;
				EventsToAdd = NextToAdd;
			}
			Curr = Next;
			Next = Next->Next;
		}
	}

	float CurrTime = (float)TimeSrv->GetTime();

	while (PendingEventsHead && PendingEventsHead->FireTime <= CurrTime)
	{
		n_assert(PendingEventsHead->Event);
		PendingEventsHead->Dispatcher->FireEvent(*PendingEventsHead->Event);
		CEventNode* Next = PendingEventsHead->Next;
		EventSrv->DestroyNode(PendingEventsHead);
		PendingEventsHead = Next;
	}
	if (!PendingEventsHead) PendingEventsTail = NULL;
}
//---------------------------------------------------------------------

}