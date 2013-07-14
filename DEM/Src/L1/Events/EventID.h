#pragma once
#ifndef __DEM_L1_EVENT_ID_H__
#define __DEM_L1_EVENT_ID_H__

#include <Core/RTTI.h>
#include <Data/StringID.h>

// Event ID standardizes the identification process for Native & Parametrized events.

namespace Events
{

using namespace Core;

struct CEventID
{
	union
	{
		const char*		ID;
		const CRTTI*	RTTI;
	};

	CEventID(): ID(NULL) {}
	CEventID(CStrID _ID): ID(_ID) {}
	CEventID(const CRTTI* _RTTI): RTTI(_RTTI) {}
	CEventID(const CRTTI& _RTTI): RTTI(&_RTTI) {}
	//???from CEvent/CEventNative/CEventBase?

	bool IsDefault() const { return ID == NULL; }

	bool operator ==(const CEventID& Other) const { return ID == Other.ID; }
	bool operator !=(const CEventID& Other) const { return ID != Other.ID; }
	bool operator <(const CEventID& Other) const { return ID < Other.ID; }
	bool operator >(const CEventID& Other) const { return ID > Other.ID; }
	bool operator <=(const CEventID& Other) const { return ID <= Other.ID; }
	bool operator >=(const CEventID& Other) const { return ID >= Other.ID; }

	operator DWORD() const { return (DWORD)ID; }
	operator CStrID() const { return *(CStrID*)&ID; }
	operator const CRTTI*() const { return RTTI; }
	operator const CRTTI&() const { return *RTTI; }
};

}

#endif