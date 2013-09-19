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
typedef bool (*CEventCallback)(const CEventBase&);

enum EEventPriority
{
	Priority_Default	= 0,		// Not set, handler will be added to the tail
	Priority_Top		= 0xffff	// Handler will be added as the head
};

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
	EventSrv->Subscribe<Class>(&Event::EventName::RTTI, this, &Class::Handler, &Sub_##EventName)

#define SUBSCRIBE_NEVENT_PRIORITY(EventName, Class, Handler, Priority) \
	EventSrv->Subscribe<Class>(&Event::EventName::RTTI, this, &Class::Handler, &Sub_##EventName, Priority)

#define SUBSCRIBE_PEVENT(EventName, Class, Handler) \
	EventSrv->Subscribe<Class>(CStrID(#EventName), this, &Class::Handler, &Sub_##EventName)

#define SUBSCRIBE_PEVENT_PRIORITY(EventName, Class, Handler, Priority) \
	EventSrv->Subscribe<Class>(CStrID(#EventName), this, &Class::Handler, &Sub_##EventName, Priority)

#define DISP_SUBSCRIBE_NEVENT(Dispatcher, EventName, Class, Handler) \
	Dispatcher->Subscribe<Class>(&Event::EventName::RTTI, this, &Class::Handler, &Sub_##EventName)

#define DISP_SUBSCRIBE_NEVENT_PRIORITY(Dispatcher, EventName, Class, Handler, Priority) \
	Dispatcher->Subscribe<Class>(&Event::EventName::RTTI, this, &Class::Handler, &Sub_##EventName, Priority)

#define DISP_SUBSCRIBE_PEVENT(Dispatcher, EventName, Class, Handler) \
	Dispatcher->Subscribe<Class>(CStrID(#EventName), this, &Class::Handler, &Sub_##EventName)

#define DISP_SUBSCRIBE_PEVENT_PRIORITY(Dispatcher, EventName, Class, Handler, Priority) \
	Dispatcher->Subscribe<Class>(CStrID(#EventName), this, &Class::Handler, &Sub_##EventName, Priority)

#define UNSUBSCRIBE_EVENT(EventName) \
	Sub_##EventName = NULL

#define IS_SUBSCRIBED(EventName) \
	(Sub_##EventName.IsValid())

#endif