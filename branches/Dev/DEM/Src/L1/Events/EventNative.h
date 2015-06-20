#pragma once
#ifndef __DEM_L1_EVENT_NATIVE_H__
#define __DEM_L1_EVENT_NATIVE_H__

#include <Events/EventBase.h>

// Native event uniquely identified by RTTI. Subclass it adding fields for parameters.
// Native events are compile-time objects that offer more performance over parametrized
// ones for the cost of additional class declaration.

// NB: this MUST be declared in all native event classes!
#define __DeclareNativeEventClass \
public: \
	static Core::CRTTI			RTTI; \
	virtual Core::CRTTI*		GetRTTI() const { return &RTTI; } \
	virtual Events::CEventID	GetID() const { return &RTTI; } \
private:

namespace Events
{

class CEventNative: public CEventBase
{
	__DeclareNativeEventClass;
};

}

#endif