#include "EventDispatcher.h"

namespace Events
{
UPTR CEventDispatcher::EventsFiredTotal = 0;

void CEventHandler::Disconnect()
{
	if (_pDispatcher)
	{
		_pDispatcher->Unsubscribe(_EventID, this);
		_pDispatcher = nullptr;
	}
}
//---------------------------------------------------------------------

UPTR CEventDispatcher::FireEvent(const CEventBase& Event, U8 Flags)
{
	UPTR HandledCounter = 0;
	CEventID EvID = Event.GetID();

	//!!!FIXME: MT! need interlocked operation for MT safety!
	++EventsFiredTotal;

	Event.UniqueNumber = EventsFiredTotal;

	// Look for this event handlers
	{
		auto It = Handlers.find(EvID);
		PEventHandler Sub = (It == Handlers.cend()) ? nullptr : It->second;
		while (Sub)
		{
			if (Sub->Invoke(this, Event))
			{
				++HandledCounter;
				if (Flags & Event_TermOnHandled) return HandledCounter;
			}
			Sub = Sub->Next;
		}
	}

	return HandledCounter;
}
//---------------------------------------------------------------------

PSub CEventDispatcher::Subscribe(PEventHandler&& Handler)
{
	n_assert_dbg(Handler && Handler->GetEventID());
	if (!Handler || !Handler->GetEventID()) return {};

	// Create before moving from Handler
	auto Conn = PSub(Handler);

	auto It = Handlers.find(Handler->GetEventID());
	if (It != Handlers.cend())
	{
		PEventHandler* pPrev = nullptr;
		PEventHandler* pCurr = &It->second;
		const auto Priority = Handler->GetPriority();
		while ((*pCurr) && (*pCurr)->GetPriority() > Priority)
		{
			pPrev = pCurr;
			pCurr = &(*pCurr)->Next;
		}

		Handler->Next = std::move(*pCurr);

		if (pPrev) (*pPrev)->Next = std::move(Handler);
		else It->second = std::move(Handler);
	}
	else Handlers.emplace(Handler->GetEventID(), std::move(Handler));

	return Conn;
}
//---------------------------------------------------------------------

void CEventDispatcher::Unsubscribe(CEventID ID, CEventHandler* pHandler)
{
	n_assert_dbg(pHandler);

	auto It = Handlers.find(ID);
	if (It == Handlers.cend())
	{
		::Sys::Error("Subscription on '{}' not found, perhaps double unsubscription occurred"_format(ID.ID));
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
