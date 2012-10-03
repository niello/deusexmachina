#pragma once
#ifndef __DEM_L1_EVENT_SUB_H__
#define __DEM_L1_EVENT_SUB_H__

#include "EventID.h"
#include "EventHandler.h"

// Subscription handle is used to unsubscribe from event

namespace Events
{

class CSubscription: public Core::CRefCounted
{
private:

	friend class CEventDispatcher;

	CEventDispatcher*	Dispatcher;
	CEventID			Event;
	PEventHandler		Handler;

	void Unsubscribe();

public:

	CSubscription(CEventDispatcher* d, CEventID e, PEventHandler h):
		Dispatcher(d), Event(e), Handler(h) {}
	virtual ~CSubscription() { Unsubscribe(); }

	CEventID				GetEvent() const { return Event; }
	const CEventHandler*	GetHandler() const { return Handler; }
	const CEventDispatcher*	GetDispatcher() const { return Dispatcher; }
};
//---------------------------------------------------------------------

typedef Ptr<CSubscription> PSub;

}

#endif