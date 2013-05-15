#pragma once
#ifndef __DEM_L1_EVENTS_H__
#define __DEM_L1_EVENTS_H__

#include <Core/Ptr.h>

// Event system declarations and helpers

namespace Events
{
	class CEventBase;
	typedef Ptr<class CSubscription> PSub;
	typedef Ptr<class CEventHandler> PEventHandler;
}

#define DECLARE_EVENT_HANDLER(EventName, HandlerName) \
	bool HandlerName(const Events::CEventBase& Event); \
	Events::PSub Sub_##EventName;

#define DECLARE_EVENT_HANDLER_VIRTUAL(EventName, HandlerName) \
	bool HandlerName##Proc(const Events::CEventBase& Event); \
	virtual void HandlerName(); \
	Events::PSub Sub_##EventName;

#define DECLARE_2_EVENTS_HANDLER(EventName1, EventName2, HandlerName) \
	bool HandlerName(const Events::CEventBase& Event); \
	Events::PSub Sub_##EventName1; \
	Events::PSub Sub_##EventName2;

#define IMPL_EVENT_HANDLER_VIRTUAL(EventName, Class, HandlerName) \
	bool Class::HandlerName##Proc(const Events::CEventBase& Event) { HandlerName(); OK; }

#define SUBSCRIBE_NEVENT(EventName, Class, Handler) \
	Sub_##EventName = EventMgr->Subscribe<Class>(&Event::EventName::RTTI, this, &Class::Handler)

#define SUBSCRIBE_PEVENT(EventName, Class, Handler) \
	Sub_##EventName = EventMgr->Subscribe<Class>(CStrID(#EventName), this, &Class::Handler)

#define DISP_SUBSCRIBE_NEVENT(Dispatcher, EventName, Class, Handler) \
	Sub_##EventName = Dispatcher->Subscribe<Class>(&Event::EventName::RTTI, this, &Class::Handler)

#define DISP_SUBSCRIBE_PEVENT(Dispatcher, EventName, Class, Handler) \
	Sub_##EventName = Dispatcher->Subscribe<Class>(CStrID(#EventName), this, &Class::Handler)

#define UNSUBSCRIBE_EVENT(EventName) \
	Sub_##EventName = NULL

#define IS_SUBSCRIBED(EventName) \
	(Sub_##EventName.IsValid())

#endif