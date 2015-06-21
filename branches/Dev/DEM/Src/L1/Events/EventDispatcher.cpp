#include "EventDispatcher.h"

#include <Events/Subscription.h>

namespace Events
{
int CEventDispatcher::EventsFiredTotal = 0;

DWORD CEventDispatcher::FireEvent(const CEventBase& Event)
{
	DWORD HandledCounter = 0;
	PEventHandler Sub;
	CEventID EvID = Event.GetID();

	// Look for subscriptions to this event
	if (Subscriptions.Get(EvID, Sub)) do
	{
		if (Sub->Invoke(this, Event))
		{
			++HandledCounter;
			if (Event.Flags & EV_TERM_ON_HANDLED) return HandledCounter;
		}
		Sub = Sub->Next;
	}
	while (Sub.IsValid());

	// Look for subscriptions to any event
	if (!(Event.Flags & EV_IGNORE_NULL_SUBS))
	{
		if (Subscriptions.Get(NULL, Sub)) do
		{
			if (Sub->Invoke(this, Event))
			{
				++HandledCounter;
				if (Event.Flags & EV_TERM_ON_HANDLED) return HandledCounter;
			}
			Sub = Sub->Next;
		}
		while (Sub.IsValid());
	}

	//!!!MT! need interlocked operation for MT safety!
	++EventsFiredTotal;

	return HandledCounter;
}
//---------------------------------------------------------------------

bool CEventDispatcher::Subscribe(CEventID ID, PEventHandler Handler, PSub* pSub)
{
	PEventHandler& CurrSlot = Subscriptions.At(ID);
	if (CurrSlot.IsValid())
	{
		PEventHandler Prev, Curr = CurrSlot;
		while (Curr.IsValid() && Curr->GetPriority() > Handler->GetPriority())
		{
			Prev = Curr;
			Curr = Curr->Next;
		}

		if (Prev.IsValid()) Prev->Next = Handler;
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
			if (Curr.GetUnsafe() == pHandler)
			{
				if (Prev.IsValid()) Prev->Next = pHandler->Next;
				else
				{
					if (pHandler->Next.IsValid()) (*pCurrSlot) = pHandler->Next;
					else Subscriptions.Remove(ID); //!!!optimize duplicate search! use CIterator!
				}
				return;
			}
			Prev = Curr;
			Curr = Curr->Next;
		}
		while (Curr.IsValid());
	}

	Sys::Error("Subscription on '%s' not found, mb double unsubscription", ID.ID);
}
//---------------------------------------------------------------------

}
