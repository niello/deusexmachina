#pragma once
#include <Data/RefCounted.h>
#include <Events/EventsFwd.h>

// Abstract wrapper for different event callback types

namespace Events
{

class CEventHandler: public Data::CRefCounted //???is unique_ptr enough?
{
protected:

	U16 Priority;

public:

	virtual ~CEventHandler() override = default;

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
