#pragma once
#ifndef __DEM_L1_EVENT_DISPATCHER_H__
#define __DEM_L1_EVENT_DISPATCHER_H__

#include <Data/HashTable.h>
#include <Events/EventHandler.h>
#include <Events/Event.h>

// Event dispatcher receives fired events and dispatches them to subordinate dispatchers and subscribers.
// Subscribers can specify their priority, and higher priority subscriber receives event first.

//???priority U16 or UPTR? does affect perf?

namespace Data
{
	typedef Ptr<class CParams> PParams;
}

namespace Events
{

class CEventDispatcher: public Core::CObject //???avoid refcounted/object inheritance? To use as mix-in. //???own refcounting inside or external lifetime control?
{
protected:

	static int EventsFiredTotal; // Can be used as an event UID

	CHashTable<CEventID, PEventHandler> Subscriptions;

public:

	CEventDispatcher(UPTR HashTableCapacity = CHashTable<CEventID, PEventHandler>::DEFAULT_SIZE): Subscriptions(HashTableCapacity) {}

	static int				GetFiredEventsCount() { return EventsFiredTotal; }

	// Returns handled counter (how much handlers have signed that they handled this event)
	UPTR					FireEvent(const CEventBase& Event);
	UPTR					FireEvent(CEventBase& Event, char Flags) { Event.Flags = Flags; return FireEvent(Event); }
	UPTR					FireEvent(CStrID ID, Data::PParams Params = NULL, char Flags = 0) { return FireEvent(CEvent(ID, Flags, Params)); }

	bool					Subscribe(CEventID ID, PEventHandler Handler, PSub* pSub = NULL);
	bool					Subscribe(CEventID ID, CEventCallback Callback, PSub* pSub = NULL, U16 Priority = Priority_Default);
	template<class T> bool	Subscribe(CEventID ID, T* Object, bool (T::*Callback)(CEventDispatcher*, const CEventBase&), PSub* pSub = NULL, U16 Priority = Priority_Default);
	void					Unsubscribe(CEventID ID, CEventHandler* pHandler);
	void					UnsubscribeAll() { Subscriptions.Clear(); }
};

typedef Ptr<CEventDispatcher> PEventDispatcher;

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