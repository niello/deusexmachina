#pragma once
#ifndef __DEM_L1_EVENT_ID_H__
#define __DEM_L1_EVENT_ID_H__

#include <Core/RTTI.h>
#include <Data/StringID.h>

// Event ID standardizes the identification process for Native & Parametrized events.
// We use the fact that any C++ object has its unique address, so all native events
// identified by RTTI and all parametrized events identified by StrIDs are unique.

namespace Events
{

struct CEventID
{
	union
	{
		const char*			ID;
		const Core::CRTTI*	RTTI;
	};

	CEventID(): ID(NULL) {}
	CEventID(CStrID _ID): ID(_ID) {}
	CEventID(const Core::CRTTI* _RTTI): RTTI(_RTTI) {}
	CEventID(const Core::CRTTI& _RTTI): RTTI(&_RTTI) {}

	bool IsDefault() const { return ID == NULL; }

	bool operator ==(const CEventID& Other) const { return ID == Other.ID; }
	bool operator !=(const CEventID& Other) const { return ID != Other.ID; }
	bool operator <(const CEventID& Other) const { return ID < Other.ID; }
	bool operator >(const CEventID& Other) const { return ID > Other.ID; }
	bool operator <=(const CEventID& Other) const { return ID <= Other.ID; }
	bool operator >=(const CEventID& Other) const { return ID >= Other.ID; }

	operator DWORD() const { return (DWORD)ID; }
	operator CStrID() const { return *(CStrID*)&ID; }
	operator const Core::CRTTI*() const { return RTTI; }
	operator const Core::CRTTI&() const { return *RTTI; }
};

}

#endif