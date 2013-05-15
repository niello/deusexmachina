#pragma once
#ifndef __DEM_L1_EVENT_BASE_H__
#define __DEM_L1_EVENT_BASE_H__

#include <Core/RefCounted.h>
#include "EventID.h"

// Event base class, abstracting fast native events (CEventNative) and flexible parametrized events (CEvent)

namespace Events
{

//???enum/consts?
#define EV_TERM_ON_HANDLED	0x01	// Stop calling handlers as one returns true = 'event is handled by me'
//???EV_NO_DEFAULT - don't send to default (NULL) handler
#define EV_ASYNC			0x08	// If flag is set event will be queued until the next frame (HandlePendingEvents call)

class CEventBase: public Core::CRefCounted
{
protected:

public:

	char Flags;

	CEventBase(): Flags(0) {}
	CEventBase(char _Flags): Flags(_Flags) {}

	virtual CEventID GetID() const = 0; //{ return NULL; }
};

}

#endif