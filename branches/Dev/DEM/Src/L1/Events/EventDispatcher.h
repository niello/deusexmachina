#pragma once
#ifndef __DEM_L1_EVENT_DISPATCHER_H__
#define __DEM_L1_EVENT_DISPATCHER_H__

#include <Data/HashTable.h>
#include <Events/EventHandler.h>
#include <Events/Event.h>
#include <Events/EventNative.h>

// Event dispatcher receives fired events and dispatches them to subordinate dispatchers and subscribers.
// Subscribers can specify their priority, and higher priority subscriber receives event first.

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

	CEventDispatcher(int HashTableCapacity = CHashTable<CEventID, PEventHandler>::DEFAULT_SIZE): Subscriptions(HashTableCapacity) {}

	static int				GetFiredEventsCount() { return EventsFiredTotal; }

	bool					AddHandler(CEventID ID, PEventHandler Handler, PSub* pSub = NULL);

	// Returns handled counter (how much handlers have signed that they handled this event)
	DWORD					FireEvent(CStrID ID, Data::PParams Params = NULL, char Flags = 0); // parametrized
	DWORD					FireEvent(CEventNative& Event, char Flags = -1); // native

	// Primarily for internal use, public because CEventServer can't call it otherwise
	DWORD					DispatchEvent(const CEventBase& Event);

	bool					Subscribe(CEventID ID, CEventCallback Callback, PSub* pSub = NULL, ushort Priority = Priority_Default);
	template<class T> bool	Subscribe(CEventID ID, T* Object, bool (T::*Callback)(const CEventBase&), PSub* pSub = NULL, ushort Priority = Priority_Default);
	void					Unsubscribe(CEventID ID, CEventHandler* pHandler);
	void					UnsubscribeAll() { Subscriptions.Clear(); }
};

typedef Ptr<CEventDispatcher> PEventDispatcher;

inline DWORD CEventDispatcher::FireEvent(CStrID ID, Data::PParams Params, char Flags)
{
	//!!!event pools!
	Ptr<CEvent> Event = n_new(CEvent)(ID, Flags, Params);
	return DispatchEvent(*Event);
}
//---------------------------------------------------------------------

inline DWORD CEventDispatcher::FireEvent(CEventNative& Event, char Flags)
{
	if (Flags != -1) Event.Flags = Flags;
	return DispatchEvent(Event);
}
//---------------------------------------------------------------------

inline bool CEventDispatcher::Subscribe(CEventID ID, CEventCallback Callback, PSub* pSub, ushort Priority)
{
	return AddHandler(ID, n_new(CEventHandlerCallback)(Callback, Priority), pSub);
}
//---------------------------------------------------------------------

template<class T>
inline bool CEventDispatcher::Subscribe(CEventID ID, T* Object, bool (T::*Callback)(const CEventBase&), PSub* pSub, ushort Priority)
{
	return AddHandler(ID, n_new(CEventHandlerMember<T>)(Object, Callback, Priority), pSub);
}
//---------------------------------------------------------------------

}

#endif