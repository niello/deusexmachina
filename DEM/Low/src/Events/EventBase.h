#pragma once
#ifndef __DEM_L1_EVENT_BASE_H__
#define __DEM_L1_EVENT_BASE_H__

#include <Core/Object.h>
#include <Events/EventID.h>

// Event base class, abstracting fast native events (CEventNative) and flexible parametrized events (CEvent)

namespace Events
{

class CEventBase: public Core::CObject
{
protected:

	mutable UPTR UniqueNumber;

	friend class CEventDispatcher; // For unique number setting

public:

	virtual CEventID	GetID() const = 0;
	UPTR				GetUniqueNumber() const { return UniqueNumber; }
};

typedef Ptr<CEventBase> PEventBase;

}

#endif