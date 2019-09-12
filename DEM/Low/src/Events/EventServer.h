#pragma once
#ifndef __DEM_L1_EVENT_SERVER_H__
#define __DEM_L1_EVENT_SERVER_H__

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
	__DeclareSingleton(CEventServer);

protected:

	struct CEventNode
	{
		float				FireTime;	//???store int or packed to int for fast CPU comparisons
		CEventDispatcher*	pDispatcher;	// Call RemoveScheduledEvents() to be able to destruct this dispatcher
		PEventBase			Event;
		CEventNode*			Next;
		U8					Flags;
		CEventNode(): Next(nullptr) {}
	};

	CPoolAllocator<CEventNode>	EventNodes;
	CEventNode*					PendingEventsHead = nullptr;
	CEventNode*					PendingEventsTail = nullptr; // To preserve events' fire order, insert to the end of the list
	CEventNode*					EventsToAdd = nullptr;

public:

	CEventServer();
	virtual ~CEventServer() override;

	void		ScheduleEvent(CEventBase& Event, U8 Flags = 0, CEventDispatcher* pDisp = nullptr, float RelTime = 0.f);
	void		ScheduleEvent(CStrID ID, Data::PParams Params = nullptr, U8 Flags = 0, CEventDispatcher* pDisp = nullptr, float RelTime = 0.f);
	UPTR		RemoveScheduledEvents(CEventDispatcher* pDisp);
	UPTR		RemoveAllScheduledEvents();

	void		ProcessPendingEvents();

	//!!!use pool inside! from map RTTI->Pool (store such mapping in Factory?)
	//!!!or use small allocator!
	//Ptr<CEventNative>			CreateNativeEvent(const Core::CRTTI* RTTI);
	//template<class T> Ptr<T>	CreateNativeEvent();
};

inline void CEventServer::ScheduleEvent(CStrID ID, Data::PParams Params, U8 Flags, CEventDispatcher* pDisp, float RelTime)
{
	//!!!event pools! can't allocate on stack here!
	Ptr<CEvent> Event = n_new(CEvent)(ID, Params);
	ScheduleEvent(*Event, Flags, pDisp, RelTime);
}
//---------------------------------------------------------------------

}

#endif
