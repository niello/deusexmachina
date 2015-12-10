#pragma once
#ifndef __DEM_L1_EVENT_MANAGER_H__
#define __DEM_L1_EVENT_MANAGER_H__

#include <Events/EventDispatcher.h>
#include <System/Allocators/PoolAllocator.h>
#include <Data/Singleton.h>

// Event server is a central coordination point for the event processing. It works as:
// - Factory/Cache, by producing event, subscription etc nodes for dispatchers' internal usage
// - Global event dispatcher if you want to send application-scope events

namespace Events
{
#define EventSrv Events::CEventServer::Instance()

class CEventServer: public CEventDispatcher
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CEventServer);

protected:

	struct CEventNode
	{
		float				FireTime;	//???store int or packed to int for fast CPU comparisons
		PEventDispatcher	Dispatcher;	// Call RemoveScheduledEvents() to be able to destruct this dispatcher
		PEventBase			Event;
		CEventNode*			Next;
		CEventNode(): Next(NULL) {}
	};

	CPoolAllocator<CEventNode>	EventNodes;
	CEventNode*					PendingEventsHead;
	CEventNode*					PendingEventsTail; // To preserve events' fire order, insert to the end of the list
	CEventNode*					EventsToAdd;

	//!!!cache pool of free events!

public:

	CEventServer();
	virtual ~CEventServer() { RemoveAllScheduledEvents(); __DestructSingleton; }

	void		ScheduleEvent(CEventBase& Event, CEventDispatcher* pDisp = NULL, float RelTime = 0.f);
	void		ScheduleEvent(CStrID ID, Data::PParams Params = NULL, char Flags = 0, CEventDispatcher* pDisp = NULL, float RelTime = 0.f);
	DWORD		RemoveScheduledEvents(CEventDispatcher* pDisp);
	DWORD		RemoveAllScheduledEvents();

	void		ProcessPendingEvents();

	CEventNode*	CreateNode() { return EventNodes.Construct(); }
	void		DestroyNode(CEventNode* pNode) { EventNodes.Destroy(pNode); }

	//!!!use pool inside! from map RTTI->Pool (store such mapping in Factory?)
	//!!!or use small allocator!
	//Ptr<CEventNative>			CreateNativeEvent(const Core::CRTTI* RTTI);
	//template<class T> Ptr<T>	CreateNativeEvent();
};

inline CEventServer::CEventServer():
	CEventDispatcher(256),
	PendingEventsHead(NULL),
	PendingEventsTail(NULL),
	EventsToAdd(NULL)
{
	__ConstructSingleton;
}
//---------------------------------------------------------------------

inline void CEventServer::ScheduleEvent(CStrID ID, Data::PParams Params, char Flags, CEventDispatcher* pDisp, float RelTime)
{
	//!!!event pools! can't allocate on stack here!
	Ptr<CEvent> Event = n_new(CEvent)(ID, Flags, Params);
	ScheduleEvent(*Event, pDisp, RelTime);
}
//---------------------------------------------------------------------

}

#endif
