#pragma once
#include <Events/EventsFwd.h>
#include <Events/EventID.h>

// Abstract wrapper for different event callback types

namespace Events
{

// FIXME: better would be to create a dispatcher node with CEventHandler inside but now the handler is a node itself to finish coding this faster
class CEventHandler : public DEM::Events::CConnectionRecordBase
{
public:

	CEventHandler(CEventDispatcher* d, CEventID e, U16 _Priority = Priority_Default) : _pDispatcher(d), _EventID(e), _Priority(_Priority) {}
	virtual ~CEventHandler() = default;

	virtual bool Invoke(CEventDispatcher* pDispatcher, const CEventBase& Event) = 0;
	bool         operator()(CEventDispatcher* pDispatcher, const CEventBase& Event) { return Invoke(pDispatcher, Event); }

	// FIXME: everything below is a node, not a callable

	PEventHandler     Next;
	U16               _Priority;
	CEventDispatcher* _pDispatcher;
	CEventID          _EventID;

	virtual bool IsConnected() const noexcept override { return !!_pDispatcher; }
	virtual void Disconnect() override; // See impl in EventDispatcher.cpp

	CEventID     GetEventID() const { return _EventID; }
	U16          GetPriority() const { return _Priority; }

};
//---------------------------------------------------------------------

class CEventHandlerCallback: public CEventHandler
{
private:

	CEventCallback Handler;

public:

	CEventHandlerCallback(CEventCallback Func, CEventDispatcher* d, CEventID e, U16 _Priority = Priority_Default): CEventHandler(d, e, _Priority), Handler(Func) {}

	virtual bool Invoke(CEventDispatcher* pDispatcher, const CEventBase& Event) { return (*Handler)(pDispatcher, Event); }
};
//---------------------------------------------------------------------

class CEventHandlerFunctor: public CEventHandler
{
private:

	CEventFunctor Handler;

public:

	CEventHandlerFunctor(CEventFunctor&& Func, CEventDispatcher* d, CEventID e, U16 _Priority = Priority_Default): CEventHandler(d, e, _Priority), Handler(std::move(Func)) {}
	CEventHandlerFunctor(const CEventFunctor& Func, CEventDispatcher* d, CEventID e, U16 _Priority = Priority_Default): CEventHandler(d, e, _Priority), Handler(Func) {}

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

	CEventHandlerMember(T* Obj, CMemberFunc Func, CEventDispatcher* d, CEventID e, U16 _Priority = Priority_Default): CEventHandler(d, e, _Priority), Object(Obj), Handler(Func) {}

	virtual bool Invoke(CEventDispatcher* pDispatcher, const CEventBase& Event) { return (Object->*Handler)(pDispatcher, Event); }
};
//---------------------------------------------------------------------

}
