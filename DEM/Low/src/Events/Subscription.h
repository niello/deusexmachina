#pragma once
#include <Data/RefCounted.h>
#include <Events/EventID.h>

// Subscription handle is handy to automatically unsubscribe from event

//???use std::unique_ptr? no need in multiple references
// TODO: pDispatcher to weak ref?

namespace Events
{
class CEventDispatcher;
class CEventHandler;

class CSubscription: public Data::CRefCounted
{
private:

	CEventDispatcher*	pDispatcher;
	CEventID			Event;
	CEventHandler*		pHandler;

public:

	CSubscription(CEventDispatcher* d, CEventID e, CEventHandler* h) : pDispatcher(d), Event(e), pHandler(h) {}
	virtual ~CSubscription() override;

	CEventID				GetEvent() const { return Event; }
	const CEventHandler*	GetHandler() const { return pHandler; }
	CEventDispatcher*		GetDispatcher() const { return pDispatcher; }
};

typedef Ptr<CSubscription> PSub;

}
