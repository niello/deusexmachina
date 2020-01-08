#pragma once
#include <Data/Ptr.h>

// Event system declarations and helpers

namespace Events
{
class CEventBase;
class CEventDispatcher;
typedef Ptr<class CSubscription> PSub;
typedef std::unique_ptr<class CEventHandler> PEventHandler;
typedef bool (*CEventCallback)(CEventDispatcher*, const CEventBase&);
typedef std::function<bool(CEventDispatcher*, const CEventBase&)> CEventFunctor;

enum EEventPriority
{
	Priority_Default	= 0,		// Not set, handler will be added to the tail
	Priority_Top		= 0xffff	// Handler will be added as the head
};

}

#define DECLARE_EVENT_HANDLER(EventName, HandlerName) \
	bool HandlerName(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event); \
	Events::PSub Sub_##EventName;

#define DECLARE_EVENT_HANDLER_VIRTUAL(EventName, HandlerName) \
	bool HandlerName##Proc(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event); \
	virtual void HandlerName(); \
	Events::PSub Sub_##EventName;

#define DECLARE_2_EVENTS_HANDLER(EventName1, EventName2, HandlerName) \
	bool HandlerName(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event); \
	Events::PSub Sub_##EventName1; \
	Events::PSub Sub_##EventName2;

#define DECLARE_ALL_EVENTS_HANDLER(HandlerName) \
	bool HandlerName(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event); \
	Events::PSub Sub_ALL_EVENTS;

#define IMPL_EVENT_HANDLER_VIRTUAL(EventName, Class, HandlerName) \
	bool Class::HandlerName##Proc(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event) { HandlerName(); OK; }

#define SUBSCRIBE_NEVENT(EventName, Class, Handler) \
	Sub_##EventName = EventSrv->Subscribe<Class>(&Event::EventName::RTTI, this, &Class::Handler)

#define SUBSCRIBE_NEVENT_PRIORITY(EventName, Class, Handler, Priority) \
	Sub_##EventName = EventSrv->Subscribe<Class>(&Event::EventName::RTTI, this, &Class::Handler, Priority)

#define SUBSCRIBE_PEVENT(EventName, Class, Handler) \
	Sub_##EventName = EventSrv->Subscribe<Class>(CStrID(#EventName), this, &Class::Handler)

#define SUBSCRIBE_PEVENT_PRIORITY(EventName, Class, Handler, Priority) \
	EventSrv->Subscribe<Class>(CStrID(#EventName), this, &Class::Handler, Priority)

#define SUBSCRIBE_ALL_EVENTS(Class, Handler) \
	Sub_##EventName = EventSrv->Subscribe<Class>(nullptr, this, &Class::Handler)

#define SUBSCRIBE_ALL_EVENTS_PRIORITY(Class, Handler, Priority) \
	Sub_##EventName = EventSrv->Subscribe<Class>(nullptr, this, &Class::Handler, Priority)

#define DISP_SUBSCRIBE_NEVENT(Dispatcher, EventName, Class, Handler) \
	Sub_##EventName = (Dispatcher)->Subscribe<Class>(&Event::EventName::RTTI, this, &Class::Handler)

#define DISP_SUBSCRIBE_NEVENT_PRIORITY(Dispatcher, EventName, Class, Handler, Priority) \
	Sub_##EventName = (Dispatcher)->Subscribe<Class>(&Event::EventName::RTTI, this, &Class::Handler, Priority)

#define DISP_SUBSCRIBE_PEVENT(Dispatcher, EventName, Class, Handler) \
	Sub_##EventName = (Dispatcher)->Subscribe<Class>(CStrID(#EventName), this, &Class::Handler)

#define DISP_SUBSCRIBE_PEVENT_PRIORITY(Dispatcher, EventName, Class, Handler, Priority) \
	Sub_##EventName = (Dispatcher)->Subscribe<Class>(CStrID(#EventName), this, &Class::Handler, Priority)

#define DISP_SUBSCRIBE_ALL_EVENTS(Dispatcher, Class, Handler) \
	Sub_##EventName = (Dispatcher)->Subscribe<Class>(nullptr, this, &Class::Handler)

#define DISP_SUBSCRIBE_ALL_EVENTS_PRIORITY(Dispatcher, Class, Handler, Priority) \
	Sub_##EventName = (Dispatcher)->Subscribe<Class>(nullptr, this, &Class::Handler, Priority)

#define UNSUBSCRIBE_EVENT(EventName) \
	Sub_##EventName = nullptr

#define IS_SUBSCRIBED(EventName) \
	(Sub_##EventName.IsValidPtr())
