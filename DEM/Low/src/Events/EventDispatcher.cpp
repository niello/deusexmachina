#include "EventDispatcher.h"
#include <Events/Subscription.h>

namespace Events
{
UPTR CEventDispatcher::EventsFiredTotal = 0;

UPTR CEventDispatcher::FireEvent(const CEventBase& Event, U8 Flags)
{
	UPTR HandledCounter = 0;
	CEventID EvID = Event.GetID();

	//!!!FIXME: MT! need interlocked operation for MT safety!
	++EventsFiredTotal;

	Event.UniqueNumber = EventsFiredTotal;

	// Look for Handlers to this event
	{
		auto It = Handlers.find(EvID);
		CEventHandler* pSub = (It == Handlers.cend()) ? nullptr : It->second.get();
		while (pSub)
		{
			if (pSub->Invoke(this, Event))
			{
				++HandledCounter;
				if (Flags & Event_TermOnHandled) return HandledCounter;
			}
			pSub = pSub->Next.get();
		}
	}

	// Look for Handlers to any event
	if (!(Flags & Event_IgnoreAllEventSubs))
	{
		auto It = Handlers.find(nullptr);
		CEventHandler* pSub = (It == Handlers.cend()) ? nullptr : It->second.get();
		while (pSub)
		{
			if (pSub->Invoke(this, Event))
			{
				++HandledCounter;
				if (Flags & Event_TermOnHandled) return HandledCounter;
			}
			pSub = pSub->Next.get();
		}
	}

	return HandledCounter;
}
//---------------------------------------------------------------------

PSub CEventDispatcher::Subscribe(CEventID ID, PEventHandler&& Handler)
{
	n_assert_dbg(Handler);

	CEventHandler* pHandler = Handler.get();
	const auto Priority = pHandler->GetPriority();

	auto It = Handlers.find(ID);
	if (It != Handlers.cend())
	{
		PEventHandler* pPrev = nullptr;
		PEventHandler* pCurr = &It->second;
		while ((*pCurr) && (*pCurr)->GetPriority() > Priority)
		{
			pPrev = pCurr;
			pCurr = &(*pCurr)->Next;
		}

		Handler->Next = std::move(*pCurr);

		if (pPrev) (*pPrev)->Next = std::move(Handler);
		else It->second = std::move(Handler);
	}
	else Handlers.emplace(ID, std::move(Handler));

	return n_new(CSubscription)(this, ID, pHandler);
}
//---------------------------------------------------------------------

void CEventDispatcher::Unsubscribe(CEventID ID, CEventHandler* pHandler)
{
	n_assert_dbg(pHandler);

	auto It = Handlers.find(ID);
	if (It == Handlers.cend())
	{
		::Sys::Error("Subscription on '%s' not found, perhaps double unsubscription occurred", ID.ID);
		return;
	}

	PEventHandler* pPrev = nullptr;
	PEventHandler* pCurr = &It->second;
	do
	{
		if ((*pCurr).get() == pHandler)
		{
			if (pPrev) (*pPrev)->Next = std::move(pHandler->Next);
			else
			{
				if (pHandler->Next) It->second = std::move(pHandler->Next);
				else Handlers.erase(It);
			}
			break;
		}
		else
		{
			pPrev = pCurr;
			pCurr = &(*pCurr)->Next;
		}
	}
	while (*pCurr);
}
//---------------------------------------------------------------------

}
