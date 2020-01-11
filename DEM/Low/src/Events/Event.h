#pragma once
#ifndef __DEM_L1_EVENT_H__
#define __DEM_L1_EVENT_H__

#include <Events/EventBase.h>
#include <Data/Params.h>
//#include <System/Allocators/PoolAllocator.h>

// Parametrized event class, can store any event and doesn't require compile-time type creation or registration

namespace Events
{

class CEvent: public CEventBase
{
	RTTI_CLASS_DECL;

protected:

	//!!!use small allocator!
	//static CPoolAllocator<CEvent, 512> Pool;

public:

	CStrID        ID;     // Event ID (string like "OnItemPicked")
	Data::PParams Params; // Event parameters

	CEvent() {}
	CEvent(CStrID _ID): ID(_ID) {}
	CEvent(CStrID _ID, Data::PParams _Params = nullptr): ID(_ID), Params(_Params) {}

	virtual CEventID GetID() const { return ID; }

//	void* operator new (size_t size) { return Pool.Allocate(); }
//	void* operator new (size_t size, void* Place) { return Place; }
//#if defined(_DEBUG) && DEM_PLATFORM_WIN32
//	void* operator new (size_t size, const char* File, int Line) { return Pool.Allocate(); }
//	void* operator new (size_t size, void* Place, const char* File, int Line) { return Place; }
//#endif
//	void operator delete (void* pObject) { Pool.Release((CEvent*)pObject); }   
};

}

#endif