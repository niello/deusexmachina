#pragma once
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
		const char*        ID = nullptr; // Pointer to string interned by CStrID
		const DEM::Core::CRTTI* RTTI;
	};

	CEventID() = default;
	CEventID(CStrID _ID): ID(_ID.CStr()) {}
	CEventID(const DEM::Core::CRTTI* _RTTI): RTTI(_RTTI) {}
	CEventID(const DEM::Core::CRTTI& _RTTI): RTTI(&_RTTI) {}

	constexpr operator bool() const noexcept { return !!ID; }

	bool operator ==(const CEventID& Other) const { return ID == Other.ID; }
	bool operator !=(const CEventID& Other) const { return ID != Other.ID; }
	bool operator <(const CEventID& Other) const { return ID < Other.ID; }
	bool operator >(const CEventID& Other) const { return ID > Other.ID; }
	bool operator <=(const CEventID& Other) const { return ID <= Other.ID; }
	bool operator >=(const CEventID& Other) const { return ID >= Other.ID; }

	operator UPTR() const { return (UPTR)ID; }
	operator CStrID() const { return *(CStrID*)&ID; }
	operator const DEM::Core::CRTTI*() const { return RTTI; }
	operator const DEM::Core::CRTTI&() const { return *RTTI; }
};

}

namespace std
{

template<>
struct hash<Events::CEventID>
{
	std::size_t operator()(Events::CEventID Value) const
	{
		// NB: the pointer hash is calculated intentionally, not a string one! See docs:
		// https://en.cppreference.com/w/cpp/utility/hash
		// There is no specialization for C strings. std::hash<const char*> produces a hash of the value
		// of the pointer (the memory address), it does not examine the contents of any character array. (c)
		return std::hash<const char*>()(Value.ID);
	}
};

}
