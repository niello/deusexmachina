#pragma once
#ifndef __DEM_L1_EVENT_HANDLER_H__
#define __DEM_L1_EVENT_HANDLER_H__

#include <Core/Object.h>
#include <Events/EventsFwd.h>

// Event handler is an abstract wrapper to event handling function (functor)

namespace Events
{

class CEventHandler: public Core::CObject
{
protected:

	U16			Priority; //???int?

public:

	PEventHandler	Next;

	CEventHandler(U16 _Priority = Priority_Default): Priority(_Priority) {}

	U16			GetPriority() const { return Priority; }
	virtual bool	Invoke(CEventDispatcher* pDispatcher, const CEventBase& Event) = 0;

	bool			operator()(CEventDispatcher* pDispatcher, const CEventBase& Event) { return Invoke(pDispatcher, Event); }
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

#endif