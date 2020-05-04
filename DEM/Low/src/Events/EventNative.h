#pragma once
#include <Events/EventBase.h>

// Native event uniquely identified by RTTI. Subclass it and add fields for parameters.
// Native events are compile-time objects and offer more performance over parametrized
// ones for the cost of additional class declaration and no runtime flexibility.

// NB: this MUST be declared in all native event classes!
#define __DeclareNativeEventClass \
public: \
	static Core::CRTTI       RTTI; \
	virtual Core::CRTTI*     GetRTTI() const { return &RTTI; } \
	virtual Events::CEventID GetID() const { return &RTTI; } \
private:

namespace Events
{

class CEventNative: public CEventBase
{
	__DeclareNativeEventClass;
};

}
