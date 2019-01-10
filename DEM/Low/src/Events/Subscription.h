#pragma once
#include <Data/RefCounted.h>
#include <Events/EventID.h>

// Subscription handle is handy to automatically unsubscribe from event
// NB: Subscription prevents its dispatcher from being destroyed (mb weak pointer would be better)

//???use std::unique_ptr? no need in multiple references

namespace Events
{
class CEventDispatcher;
typedef Ptr<class CEventHandler> PEventHandler;

class CSubscription: public Data::CRefCounted
{
private:

	CEventDispatcher*	pDispatcher;
	CEventID			Event;
	PEventHandler		Handler;

public:

	CSubscription(CEventDispatcher* d, CEventID e, PEventHandler h);
	virtual ~CSubscription();

	CEventID				GetEvent() const { return Event; }
	const CEventHandler*	GetHandler() const { return Handler; }
	const CEventDispatcher*	GetDispatcher() const { return pDispatcher; }
};

typedef Ptr<CSubscription> PSub;

}
