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
#define EventSrv ::Events::CEventServer::Instance()

class CEventServer: public CEventDispatcher
{
	__DeclareSingleton(CEventServer);

protected:

	struct CEventNode
	{
		float				FireTime = 0.f;	//???store int or packed to int for fast CPU comparisons
		CEventDispatcher*	pDispatcher = nullptr;	// Call RemoveScheduledEvents() to be able to destruct this dispatcher
		PEventBase			Event;
		CEventNode*			Next = nullptr;
		U8					Flags = 0;
	};

	CPool<CEventNode> EventNodes;
	CEventNode*       PendingEventsHead = nullptr;
	CEventNode*       PendingEventsTail = nullptr; // To preserve events' fire order, insert to the end of the list
	CEventNode*       EventsToAdd = nullptr;

public:

	CEventServer();
	virtual ~CEventServer() override;

	void		ScheduleEvent(PEventBase&& Event, U8 Flags = 0, CEventDispatcher* pDisp = nullptr, float RelTime = 0.f);
	void		ScheduleEvent(CStrID ID, Data::PParams Params = nullptr, U8 Flags = 0, CEventDispatcher* pDisp = nullptr, float RelTime = 0.f);
	UPTR		RemoveScheduledEvents(CEventDispatcher* pDisp);
	UPTR		RemoveAllScheduledEvents();

	void		ProcessPendingEvents();
};

inline void CEventServer::ScheduleEvent(CStrID ID, Data::PParams Params, U8 Flags, CEventDispatcher* pDisp, float RelTime)
{
	ScheduleEvent(std::make_unique<CEvent>(ID, Params), Flags, pDisp, RelTime);
}
//---------------------------------------------------------------------

}

#endif
