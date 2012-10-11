#pragma once
#ifndef __DEM_L1_EVENT_DISPATCHER_H__
#define __DEM_L1_EVENT_DISPATCHER_H__

#include <util/HashTable.h>
#include "Subscription.h"
#include "Event.h"
#include "EventNative.h"

// Event dispatcher receives fired events and dispatches them to subordinate dispatchers and subscribers.
// Subscribers can specify their priority, and higher priority subscriber receives event first.

namespace Data
{
	typedef Ptr<class CParams> PParams;
}

namespace Events
{
using namespace Data;

class CEventDispatcher: public Core::CRefCounted
{
protected:

	struct CEventNode
	{
		float			FireTime; //???store int or packed to int for CPU fast
		Ptr<CEventBase>	Event;
		CEventNode*		Next;
		CEventNode(): Next(NULL) {}
	};

	CEventNode* PendingEventsHead;
	CEventNode* PendingEventsTail; // to preserve events' fire order, insert to the end of the list
	CEventNode* EventsToAdd;

	// can use sorted array instead of nList & implement subscription priority
	HashTable<CEventID, PEventHandler> Subscriptions;

	DWORD	ScheduleEvent(CEventBase* Event, float RelTime);
	DWORD	DispatchEvent(const CEventBase& Event);

	// Event handler (to subscribe to other dispatcher and receive its events)
	bool	OnEvent(const CEventBase& Event) { return !!DispatchEvent(Event); }

public:

	CEventDispatcher();
	CEventDispatcher(int HashTableCapacity);
	virtual ~CEventDispatcher();

	PSub					AddHandler(CEventID ID, PEventHandler Handler);

	// Returns handled counter (how much handlers handled this event)
	DWORD					FireEvent(CStrID ID, PParams Params = NULL, char Flags = 0, float RelTime = 0.f); /// non-native
	DWORD					FireEvent(CEventNative& Event, char Flags = -1, float RelTime = 0.f); /// native

	void					ProcessPendingEvents();

	// Return value is subscription handle used to unsubscribe
	//???leave 2 instead of 4 with CEventID first param?
	PSub					Subscribe(CStrID ID, bool (*Callback)(const CEventBase&), ushort Priority = Priority_Default);
	template<class T> PSub	Subscribe(CStrID ID, T* Object, bool (T::*Callback)(const CEventBase&), ushort Priority = Priority_Default);
	PSub					Subscribe(const CRTTI* RTTI, bool (*Callback)(const CEventBase&), ushort Priority = Priority_Default);
	template<class T> PSub	Subscribe(const CRTTI* RTTI, T* Object, bool (T::*Callback)(const CEventBase&), ushort Priority = Priority_Default);
	PSub					Subscribe(CEventDispatcher& Listener, ushort Priority = Priority_Default);

	void					Unsubscribe(PSub Sub);
	void					Unsubscribe(CEventID ID, CEventHandler* Handler);
	//void					UnsubscribeEvent(CStrID ID);
	//void					UnsubscribeEvent(const CRTTI* RTTI);
	//void					UnsubscribeCallback(bool (*Callback)(const CEventBase&));
	//template<class T> void	UnsubscribeObject(T* Object);
	//template<class T> void	Unsubscribe(CStrID ID, T* Object, bool FirstOnly = true);
	void					UnsubscribeAll() { Subscriptions.Clear(); }
};
//---------------------------------------------------------------------

inline CEventDispatcher::CEventDispatcher():
	PendingEventsHead(NULL),
	PendingEventsTail(NULL),
	EventsToAdd(NULL)
{
}
//---------------------------------------------------------------------

inline CEventDispatcher::CEventDispatcher(int HashTableCapacity):
	Subscriptions(HashTableCapacity),
	PendingEventsHead(NULL),
	PendingEventsTail(NULL),
	EventsToAdd(NULL)
{
}
//---------------------------------------------------------------------

inline DWORD CEventDispatcher::FireEvent(CStrID ID, PParams Params, char Flags, float RelTime)
{
	//!!!event pools!
	Ptr<CEvent> Event = n_new(CEvent)(ID, Flags, Params); //EventMgr->ParamEvents.Allocate();
	return ScheduleEvent(Event, RelTime);
}
//---------------------------------------------------------------------

inline DWORD CEventDispatcher::FireEvent(CEventNative& Event, char Flags, float RelTime)
{
	if (Flags != -1) Event.Flags = Flags;
	return ScheduleEvent(&Event, RelTime);
}
//---------------------------------------------------------------------

inline PSub CEventDispatcher::Subscribe(CStrID ID, CEventCallback Callback, ushort Priority)
{
	return AddHandler(ID, n_new(CEventHandlerCallback)(Callback, Priority));
}
//---------------------------------------------------------------------

template<class T>
inline PSub CEventDispatcher::Subscribe(CStrID ID, T* Object, bool (T::*Callback)(const CEventBase&), ushort Priority)
{
	return AddHandler(ID, n_new(CEventHandlerMember<T>)(Object, Callback, Priority));
}
//---------------------------------------------------------------------

inline PSub CEventDispatcher::Subscribe(const CRTTI* RTTI, CEventCallback Callback, ushort Priority)
{
	return AddHandler(RTTI, n_new(CEventHandlerCallback)(Callback, Priority));
}
//---------------------------------------------------------------------

template<class T>
inline PSub CEventDispatcher::Subscribe(const CRTTI* RTTI, T* Object, bool (T::*Callback)(const CEventBase&), ushort Priority)
{
	return AddHandler(RTTI, n_new(CEventHandlerMember<T>)(Object, Callback, Priority));
}
//---------------------------------------------------------------------

inline PSub CEventDispatcher::Subscribe(CEventDispatcher& Listener, ushort Priority)
{
	return AddHandler(NULL, n_new(CEventHandlerMember<CEventDispatcher>)(&Listener, &CEventDispatcher::OnEvent, Priority));
}
//---------------------------------------------------------------------

inline void CEventDispatcher::Unsubscribe(PSub Sub)
{
	n_assert(Sub->Dispatcher == this);
	Unsubscribe(Sub->Event, Sub->Handler);
}
//---------------------------------------------------------------------

}

#endif