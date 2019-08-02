#include "EventDispatcher.h"

#include <Events/Subscription.h>

namespace Events
{
UPTR CEventDispatcher::EventsFiredTotal = 0;

UPTR CEventDispatcher::FireEvent(const CEventBase& Event, U8 Flags)
{
	UPTR HandledCounter = 0;
	PEventHandler Sub;
	CEventID EvID = Event.GetID();

	//!!!MT! need interlocked operation for MT safety!
	++EventsFiredTotal;

	Event.UniqueNumber = EventsFiredTotal;

	// Look for subscriptions to this event
	if (Subscriptions.Get(EvID, Sub)) do
	{
		if (Sub->Invoke(this, Event))
		{
			++HandledCounter;
			if (Flags & Event_TermOnHandled) return HandledCounter;
		}
		Sub = Sub->Next;
	}
	while (Sub.IsValidPtr());

	// Look for subscriptions to any event
	if (!(Flags & Event_IgnoreAllEventSubs))
	{
		if (Subscriptions.Get(NULL, Sub)) do
		{
			if (Sub->Invoke(this, Event))
			{
				++HandledCounter;
				if (Flags & Event_TermOnHandled) return HandledCounter;
			}
			Sub = Sub->Next;
		}
		while (Sub.IsValidPtr());
	}

	return HandledCounter;
}
//---------------------------------------------------------------------

bool CEventDispatcher::Subscribe(CEventID ID, PEventHandler Handler, PSub* pSub)
{
	PEventHandler& CurrSlot = Subscriptions.At(ID);
	if (CurrSlot.IsValidPtr())
	{
		PEventHandler Prev, Curr = CurrSlot;
		while (Curr.IsValidPtr() && Curr->GetPriority() > Handler->GetPriority())
		{
			Prev = Curr;
			Curr = Curr->Next;
		}

		if (Prev.IsValidPtr()) Prev->Next = Handler;
		else CurrSlot = Handler;
		Handler->Next = Curr;
	}
	else CurrSlot = Handler;
	if (pSub) *pSub = n_new(CSubscription)(this, ID, Handler);
	OK;
}
//---------------------------------------------------------------------

void CEventDispatcher::Unsubscribe(CEventID ID, CEventHandler* pHandler)
{
	n_assert_dbg(pHandler);

	PEventHandler* pCurrSlot = Subscriptions.Get(ID);
	if (pCurrSlot)
	{
		PEventHandler Prev, Curr = *pCurrSlot;
		do
		{
			if (Curr.Get() == pHandler)
			{
				if (Prev.IsValidPtr()) Prev->Next = pHandler->Next;
				else
				{
					if (pHandler->Next.IsValidPtr()) (*pCurrSlot) = pHandler->Next;
					else Subscriptions.Remove(ID); //!!!optimize duplicate search! use CIterator!
				}
				return;
			}
			Prev = Curr;
			Curr = Curr->Next;
		}
		while (Curr.IsValidPtr());
	}

	Sys::Error("Subscription on '%s' not found, mb double unsubscription", ID.ID);
}
//---------------------------------------------------------------------

}
