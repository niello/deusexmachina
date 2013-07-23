#pragma once
#ifndef __DEM_L1_EVENT_SUB_H__
#define __DEM_L1_EVENT_SUB_H__

#include <Events/EventDispatcher.h>

// Subscription handle is handy to automatically unsubscribe from event
// NB: Subscription prevents its dispatcher from being destroyed (mb weak pointer would be better)

namespace Events
{

class CSubscription: public Core::CRefCounted
{
private:

	PEventDispatcher	Dispatcher; // Sometimes dispatcher is deleted while some subscriptions exist. Use weak ptr?
	CEventID			Event;
	PEventHandler		Handler;

public:

	CSubscription(CEventDispatcher* d, CEventID e, PEventHandler h):
		Dispatcher(d), Event(e), Handler(h) {}
	virtual ~CSubscription() { Dispatcher->Unsubscribe(Event, Handler); }

	CEventID				GetEvent() const { return Event; }
	const CEventHandler*	GetHandler() const { return Handler; }
	const CEventDispatcher*	GetDispatcher() const { return Dispatcher; }
};

typedef Ptr<CSubscription> PSub;

}

#endif