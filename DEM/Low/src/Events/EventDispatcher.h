#pragma once
#include <Events/EventHandler.h>
#include <Events/Event.h>

// Event dispatcher receives fired events and dispatches them to subordinate dispatchers and subscribers.
// Subscribers can specify their priority, and higher priority subscriber receives event first.
// Use it as a mix-in class.

namespace Data
{
	using PParams = Ptr<class CParams>;
}

namespace Events
{

enum EEventFlags
{
	Event_TermOnHandled = 0x01,	// Stop calling handlers as one returns true, which means 'event is handled by me'
};

class CEventDispatcher
{
protected:

	static UPTR EventsFiredTotal; // Can be used as an event UID

	std::unordered_map<CEventID, PEventHandler> Handlers;

	virtual ~CEventDispatcher() = default; // Object should never be deleted by the pointer to a mix-in part

public:

	static UPTR	GetFiredEventsCount() { return EventsFiredTotal; }

	// Returns handled counter (how much handlers have signed that they handled this event)
	UPTR		FireEvent(const CEventBase& Event, U8 Flags = 0);
	UPTR		FireEvent(CStrID ID, Data::PParams Params = nullptr, U8 Flags = 0) { return FireEvent(CEvent(ID, Params), Flags); }

	PSub		Subscribe(PEventHandler&& Handler);
	PSub		Subscribe(CEventID ID, CEventCallback Callback, U16 Priority = Priority_Default);
	PSub		Subscribe(CEventID ID, CEventFunctor&& Functor, U16 Priority = Priority_Default);
	PSub		Subscribe(CEventID ID, const CEventFunctor& Functor, U16 Priority = Priority_Default);

	template<class T>
	PSub		Subscribe(CEventID ID, T* Object, bool (T::*Callback)(CEventDispatcher*, const CEventBase&), U16 Priority = Priority_Default);

	void		Unsubscribe(CEventID ID, CEventHandler* pHandler);
	void		UnsubscribeAll() { Handlers.clear(); }
};

inline PSub CEventDispatcher::Subscribe(CEventID ID, CEventCallback Callback, U16 Priority)
{
	return ID ? Subscribe(std::make_shared<CEventHandlerCallback>(Callback, this, ID, Priority)) : PSub{};
}
//---------------------------------------------------------------------

inline PSub CEventDispatcher::Subscribe(CEventID ID, CEventFunctor&& Functor, U16 Priority)
{
	return ID ? Subscribe(std::make_shared<CEventHandlerFunctor>(std::move(Functor), this, ID, Priority)) : PSub{};
}
//---------------------------------------------------------------------

inline PSub CEventDispatcher::Subscribe(CEventID ID, const CEventFunctor& Functor, U16 Priority)
{
	return ID ? Subscribe(std::make_shared<CEventHandlerFunctor>(Functor, this, ID, Priority)) : PSub{};
}
//---------------------------------------------------------------------

template<class T>
inline PSub CEventDispatcher::Subscribe(CEventID ID, T* Object, bool (T::*Callback)(CEventDispatcher*, const CEventBase&), U16 Priority)
{
	return ID ? Subscribe(std::make_shared<CEventHandlerMember<T>>(Object, Callback, this, ID, Priority)) : PSub{};
}
//---------------------------------------------------------------------

}
