#pragma once
#include <Data/RefCounted.h>
#include <Events/EventsFwd.h>

// Abstract wrapper for different event callback types

namespace Events
{

// NB: refcounting is required for unsubscribing from an event inside its handler
class CEventHandler
{
protected:

	U16 RefCount = 0;
	U16 Priority;

	friend inline void DEMPtrAddRef(Events::CEventHandler* p) noexcept { ++p->RefCount; }
	friend inline void DEMPtrRelease(Events::CEventHandler* p) noexcept { n_assert_dbg(p->RefCount > 0); if (--p->RefCount == 0) n_delete(p); }

public:

	virtual ~CEventHandler() = default;

	PEventHandler Next;

	CEventHandler(U16 _Priority = Priority_Default): Priority(_Priority) {}

	U16          GetPriority() const { return Priority; }
	virtual bool Invoke(CEventDispatcher* pDispatcher, const CEventBase& Event) = 0;

	bool         operator()(CEventDispatcher* pDispatcher, const CEventBase& Event) { return Invoke(pDispatcher, Event); }
};
//---------------------------------------------------------------------

class CEventHandlerCallback: public CEventHandler
{
private:

	CEventCallback Handler;

public:

	CEventHandlerCallback(CEventCallback Func, U16 _Priority = Priority_Default): CEventHandler(_Priority), Handler(Func) {}

	virtual bool Invoke(CEventDispatcher* pDispatcher, const CEventBase& Event) { return (*Handler)(pDispatcher, Event); }
};
//---------------------------------------------------------------------

class CEventHandlerFunctor: public CEventHandler
{
private:

	CEventFunctor Handler;

public:

	CEventHandlerFunctor(CEventFunctor&& Func, U16 _Priority = Priority_Default): CEventHandler(_Priority), Handler(std::move(Func)) {}
	CEventHandlerFunctor(const CEventFunctor& Func, U16 _Priority = Priority_Default): CEventHandler(_Priority), Handler(Func) {}

	virtual bool Invoke(CEventDispatcher* pDispatcher, const CEventBase& Event) { return Handler(pDispatcher, Event); }
};
//---------------------------------------------------------------------

template<typename T>
class CEventHandlerMember: public CEventHandler
{
private:

	typedef bool(T::*CMemberFunc)(CEventDispatcher* pDispatcher, const CEventBase& Event);

	T*			Object;
	CMemberFunc	Handler;

public:

	CEventHandlerMember(T* Obj, CMemberFunc Func, U16 _Priority = Priority_Default): CEventHandler(_Priority), Object(Obj), Handler(Func) {}

	virtual bool Invoke(CEventDispatcher* pDispatcher, const CEventBase& Event) { return (Object->*Handler)(pDispatcher, Event); }
};
//---------------------------------------------------------------------

}
