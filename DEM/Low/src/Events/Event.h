#pragma once
#include <Events/EventBase.h>
#include <Data/Params.h>
//#include <System/Allocators/PoolAllocator.h>

// Parametrized event class, can store any event and doesn't require compile-time type creation or registration

namespace Events
{

class CEvent: public CEventBase
{
	RTTI_CLASS_DECL(Events::CEvent, Events::CEventBase);

protected:

	//!!!use small allocator!
	//static CPoolAllocator<CEvent, 512> Pool;

public:

	CStrID        ID;     // Event ID (string like "OnItemPicked")
	Data::PParams Params; // Event parameters

	CEvent() = default;
	CEvent(CStrID ID_): ID(ID_) {}
	CEvent(CStrID ID_, Data::PParams Params_ = nullptr): ID(ID_), Params(std::move(Params_)) {}

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
