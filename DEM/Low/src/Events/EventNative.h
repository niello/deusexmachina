#pragma once
#include <Events/EventBase.h>

// Native event uniquely identified by RTTI. Subclass it and add fields for parameters.
// Native events are compile-time objects and offer more performance over parametrized
// ones for the cost of additional class declaration and no runtime flexibility.

// NB: this MUST be declared in all native event classes!
#define NATIVE_EVENT_DECL(Class, ParentClass) \
public: \
	inline static const ::Core::CRTTI RTTI = ::Core::CRTTI(#Class, 0, nullptr, &ParentClass::RTTI, 0); \
	virtual const ::Core::CRTTI* GetRTTI() const override { return &RTTI; } \
	virtual Events::CEventID     GetID() const override { return &RTTI; } \
private:

namespace Events
{

class CEventNative: public CEventBase
{
	NATIVE_EVENT_DECL(CEventNative, Events::CEventBase);
};

}
