#pragma once
#ifndef __DEM_L1_EVENT_MANAGER_H__
#define __DEM_L1_EVENT_MANAGER_H__

#include "EventDispatcher.h"
#include "EventBase.h"
#include <Data/Pool.h>

// Event manager is a central coordination point for the event processing. It works as:
// - Factory/Cache, by producing event, subscription etc nodes for dispatchers' internal usage
// - Global event dispatcher if you want to send application-scope events

namespace Data
{
	class CParams;
}

namespace Events
{

using namespace Data;

#define EventMgr Events::CEventManager::Instance()

class CEventManager: public CEventDispatcher
{
	DeclareRTTI;
	__DeclareSingleton(CEventManager);

public:

	// For internal usage
	CPool<CEventNode>		EventNodes;

	//!!!cache pool of free events, get from here if can
	//or store cache pools in CEventManager

	CEventManager();
	virtual ~CEventManager();

	//!!!use pool inside!
	//Ptr<CEventNative>			CreateNativeEvent(const Core::CRTTI* RTTI);
	//template<class T> Ptr<T>	CreateNativeEvent();
};

}

#endif
