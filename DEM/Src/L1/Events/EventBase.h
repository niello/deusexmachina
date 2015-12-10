#pragma once
#ifndef __DEM_L1_EVENT_BASE_H__
#define __DEM_L1_EVENT_BASE_H__

#include <Core/Object.h>
#include <Events/EventID.h>

// Event base class, abstracting fast native events (CEventNative) and flexible parametrized events (CEvent)

namespace Events
{
#define EV_TERM_ON_HANDLED	0x01	// Stop calling handlers as one returns true, which means 'event is handled by me'
#define EV_IGNORE_NULL_SUBS	0x02	// Don't send to default (any-event, NULL) handlers

class CEventBase: public Core::CObject
{
public:

	char Flags;

	CEventBase(): Flags(0) {}
	CEventBase(char _Flags): Flags(_Flags) {}

	virtual CEventID GetID() const = 0;
};

typedef Ptr<CEventBase> PEventBase;

}

#endif