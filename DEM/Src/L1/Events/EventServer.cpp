#include "EventServer.h"

#include <Time/TimeServer.h>

namespace Events
{
__ImplementClassNoFactory(Events::CEventServer, Core::CObject);
__ImplementSingleton(Events::CEventServer);

void CEventServer::ScheduleEventInternal(CEventBase& Event, CEventDispatcher* pDisp, float RelTime)
{
	CEventNode* New = EventSrv->CreateNode();
	n_assert2(New, "Nervous system of the engine was paralyzed! Can't allocate event node");
	New->Event = &Event;
	New->Dispatcher = pDisp ? pDisp : this;
	New->FireTime = (float)TimeSrv->GetTime() + RelTime;
	if (PendingEventsTail)
	{
		n_assert(PendingEventsHead);

		if (New->FireTime >= PendingEventsTail->FireTime)
		{
			PendingEventsTail->Next = New;
			PendingEventsTail = New;
		}
		else
		{
			if (PendingEventsHead == PendingEventsTail)
			{
				PendingEventsHead = New;
				PendingEventsHead->Next = PendingEventsTail;
			}
			else if (EventsToAdd)
			{
				CEventNode* Curr = EventsToAdd;
				CEventNode* Next = EventsToAdd->Next;
				while (Curr)
				{
					if (New->FireTime >= Curr->FireTime)
					{
						Curr->Next = New;
						New->Next = Next;
						break;
					}
					Curr = Next;
					Next = Next->Next;
				}
			}
			else EventsToAdd = New;
		}
	}
	else
	{
		n_assert(!PendingEventsHead);
		PendingEventsHead =
		PendingEventsTail = New;
	}
}
//---------------------------------------------------------------------

DWORD CEventServer::RemoveScheduledEvents(CEventDispatcher* pDisp)
{
	CEventDispatcher* pRealDisp = pDisp ? pDisp : this;
	DWORD Total = 0;

	Sys::Error("IMPLEMENT ME!!!");

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
			n_assert(Next); // will always be because insertions to 1-elm list are done immediately on queuing above
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