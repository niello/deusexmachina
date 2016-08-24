#include "EventServer.h"

#include <Time/TimeServer.h>

namespace Events
{
__ImplementSingleton(Events::CEventServer);

void CEventServer::ScheduleEvent(CEventBase& Event, U8 Flags, CEventDispatcher* pDisp, float RelTime)
{
	CEventNode* pNewNode = EventNodes.Construct();
	n_assert2(pNewNode, "Nervous system of the engine was paralyzed! Can't allocate event node");
	pNewNode->FireTime = (float)TimeSrv->GetTime() + RelTime;
	pNewNode->Event = &Event;
	pNewNode->pDispatcher = pDisp ? pDisp : this;
	pNewNode->Flags = Flags;
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

UPTR CEventServer::RemoveScheduledEvents(CEventDispatcher* pDisp)
{
	CEventDispatcher* pRealDisp = pDisp ? pDisp : this;
	UPTR Total = 0;

	NOT_IMPLEMENTED;

	return Total;
}
//---------------------------------------------------------------------

UPTR CEventServer::RemoveAllScheduledEvents()
{
	UPTR Total = 0;

	while (PendingEventsHead)
	{
		CEventNode* Next = PendingEventsHead->Next;
		EventNodes.Destroy(PendingEventsHead);
		PendingEventsHead = Next;
		++Total;
	}

	while (EventsToAdd)
	{
		CEventNode* Next = EventsToAdd->Next;
		EventNodes.Destroy(EventsToAdd);
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
		PendingEventsHead->pDispatcher->FireEvent(*PendingEventsHead->Event, PendingEventsHead->Flags);
		CEventNode* Next = PendingEventsHead->Next;
		EventNodes.Destroy(PendingEventsHead);
		PendingEventsHead = Next;
	}
	if (!PendingEventsHead) PendingEventsTail = NULL;
}
//---------------------------------------------------------------------

}