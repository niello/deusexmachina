#pragma once
#ifndef __DEM_L1_EVENT_MANAGER_H__
#define __DEM_L1_EVENT_MANAGER_H__

#include "EventDispatcher.h"
#include <Data/Pool.h>
#include <Data/Singleton.h>

// Event server is a central coordination point for the event processing. It works as:
// - Factory/Cache, by producing event, subscription etc nodes for dispatchers' internal usage
// - Global event dispatcher if you want to send application-scope events

namespace Data
{
	class CParams;
}

namespace Events
{
#define EventSrv Events::CEventServer::Instance()

class CEventServer: public CEventDispatcher
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CEventServer);

protected:

	CPoolAllocator<CEventNode>	EventNodes;

	//!!!cache pool of free events, get from here if can
	//or store cache pools in CEventServer

	int					EventsFiredTotal;

public:

	CEventServer(): CEventDispatcher(256), EventsFiredTotal(0) { __ConstructSingleton; }
	virtual ~CEventServer() { Clear(); __DestructSingleton; }

	CEventNode*	CreateNode() { return EventNodes.Construct(); }
	void		DestroyNode(CEventNode* pNode) { EventNodes.Destroy(pNode); }

	void		IncrementFiredEventsCount() { ++EventsFiredTotal; }
	int			GetFiredEventsCount() const { return EventsFiredTotal; }

	//!!!use pool inside! from map RTTI->Pool (store such mapping in Factory?)
	//Ptr<CEventNative>			CreateNativeEvent(const Core::CRTTI* RTTI);
	//template<class T> Ptr<T>	CreateNativeEvent();
};

}

#endif
