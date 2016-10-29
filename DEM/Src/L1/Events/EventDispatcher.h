#pragma once
#ifndef __DEM_L1_EVENT_DISPATCHER_H__
#define __DEM_L1_EVENT_DISPATCHER_H__

#include <Data/HashTable.h>
#include <Events/EventHandler.h>
#include <Events/Event.h>

// Event dispatcher receives fired events and dispatches them to subordinate dispatchers and subscribers.
// Subscribers can specify their priority, and higher priority subscriber receives event first.
// Use it as a mix-in class.

namespace Data
{
	typedef Ptr<class CParams> PParams;
}

namespace Events
{

enum EEventFlags
{
	Event_TermOnHandled		= 0x01,	// Stop calling handlers as one returns true, which means 'event is handled by me'
	Event_IgnoreNULLSubs	= 0x02	// Don't send to default (any-event, NULL) handlers
};

class CEventDispatcher
{
protected:

	static UPTR EventsFiredTotal; // Can be used as an event UID

	CHashTable<CEventID, PEventHandler> Subscriptions;

public:

	CEventDispatcher(UPTR HashTableCapacity = CHashTable<CEventID, PEventHandler>::DEFAULT_SIZE): Subscriptions(HashTableCapacity) {}
	virtual ~CEventDispatcher() {}

	static UPTR				GetFiredEventsCount() { return EventsFiredTotal; }

	// Returns handled counter (how much handlers have signed that they handled this event)
	UPTR					FireEvent(const CEventBase& Event, U8 Flags = 0);
	UPTR					FireEvent(CStrID ID, Data::PParams Params = NULL, U8 Flags = 0) { return FireEvent(CEvent(ID, Params), Flags); }

	bool					Subscribe(CEventID ID, PEventHandler Handler, PSub* pSub = NULL);
	bool					Subscribe(CEventID ID, CEventCallback Callback, PSub* pSub = NULL, U16 Priority = Priority_Default);
	template<class T> bool	Subscribe(CEventID ID, T* Object, bool (T::*Callback)(CEventDispatcher*, const CEventBase&), PSub* pSub = NULL, U16 Priority = Priority_Default);
	void					Unsubscribe(CEventID ID, CEventHandler* pHandler);
	void					UnsubscribeAll() { Subscriptions.Clear(); }
};

inline bool CEventDispatcher::Subscribe(CEventID ID, CEventCallback Callback, PSub* pSub, U16 Priority)
{
	return Subscribe(ID, n_new(CEventHandlerCallback)(Callback, Priority), pSub);
}
//---------------------------------------------------------------------

template<class T>
inline bool CEventDispatcher::Subscribe(CEventID ID, T* Object, bool (T::*Callback)(CEventDispatcher*, const CEventBase&), PSub* pSub, U16 Priority)
{
	return Subscribe(ID, n_new(CEventHandlerMember<T>)(Object, Callback, Priority), pSub);
}
//---------------------------------------------------------------------

}

#endif