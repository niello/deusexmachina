#pragma once
#ifndef __DEM_L1_EVENT_HANDLER_H__
#define __DEM_L1_EVENT_HANDLER_H__

#include <Core/RefCounted.h>

// Event handler is an abstract wrapper to event handling function (functor)

namespace Events
{
class CEventBase;

typedef bool(*CEventCallback)(const CEventBase& Event);
typedef Ptr<class CEventHandler> PEventHandler;

enum EEventPriority
{
	Priority_Default	= 0,		// Not set, handler will be added to the tail
	Priority_Top		= 0xffff	// Handler will be added as the head
};

class CEventHandler: public Core::CRefCounted
{
protected:

	ushort			Priority; //???int?

public:

	PEventHandler	Next;

	CEventHandler(ushort _Priority = Priority_Default): Priority(_Priority) {}

	ushort			GetPriority() const { return Priority; }

	virtual bool	operator()(const CEventBase& Event) = 0;
};
//---------------------------------------------------------------------

class CEventHandlerCallback: public CEventHandler
{
private:

	CEventCallback Handler;

public:

	CEventHandlerCallback(CEventCallback Func, ushort _Priority = Priority_Default): CEventHandler(_Priority), Handler(Func) {}

	virtual bool operator()(const CEventBase& Event) { return (*Handler)(Event); }
};
//---------------------------------------------------------------------

template<typename T>
class CEventHandlerMember: public CEventHandler
{
private:

	typedef bool(T::*CMemberFunc)(const CEventBase& Event);

	T*			Object;
	CMemberFunc	Handler;

public:

	CEventHandlerMember(T* Obj, CMemberFunc Func, ushort _Priority = Priority_Default): CEventHandler(_Priority), Object(Obj), Handler(Func) {}

	virtual bool operator()(const CEventBase& Event) { return (Object->*Handler)(Event); }
};
//---------------------------------------------------------------------

}

#endif