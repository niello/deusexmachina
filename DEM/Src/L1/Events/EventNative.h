#pragma once
#ifndef __DEM_L1_EVENT_NATIVE_H__
#define __DEM_L1_EVENT_NATIVE_H__

#include "EventBase.h"

// Native event. Subclass it adding fields for parameters. Remember to redefine RTTI in the derivatives!

namespace Events
{

class CEventNative: public CEventBase
{
	__DeclareClassNoFactory;

protected:


public:

	//CEventNative() {}

	/// To avoid 2 virtual calls override in subclass by returning &RTTI directly
	virtual CEventID GetID() const { return GetRTTI(); }
};

}

#endif