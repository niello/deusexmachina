#pragma once
#include <Core/RTTIBaseClass.h>
#include <Events/EventID.h>

// Event base class, abstracting fast native events (CEventNative) and flexible parametrized events (CEvent)

//???add handled counter inside? add flags inside? for handlers to read

namespace Events
{

class CEventBase : public Core::CRTTIBaseClass
{
protected:

	mutable UPTR UniqueNumber = 0;

	friend class CEventDispatcher; // For unique number setting

public:

	virtual CEventID GetID() const = 0;
	UPTR             GetUniqueNumber() const { return UniqueNumber; }
};

typedef std::unique_ptr<CEventBase> PEventBase;

}
