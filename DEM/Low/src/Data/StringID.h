#pragma once
#include <Data/Hash.h>
#include <string>

// Static string identifier. The actual string is stored only once and all CStrIDs reference it
// by pointers. That is guaranteed that each string (case-sensitive) will have its unique address
// and all CStrIDs created from this string are the same.
// CStrIDs can be compared as integers, but still store informative string data inside.

namespace Data
{

class CStringID
{
protected:

	friend class CStringIDStorage;

	const char* pString = nullptr;

	explicit CStringID(const char* pStr, int, int) { pString = pStr; }

public:

	static const CStringID Empty;

	CStringID() = default;
	explicit CStringID(std::string_view Str, bool OnlyExisting = false);
	CStringID(const CStringID& Other) = default;
	CStringID(CStringID&& Other) noexcept = default;
	CStringID& operator =(const CStringID& Other) = default;
	CStringID& operator =(CStringID&& Other) noexcept = default;

	uintptr_t	GetID() const { return reinterpret_cast<uintptr_t>(pString); }
	const char*	CStr() const { return pString; }
	std::string ToString() const { return pString ? std::string(pString) : std::string(); }
	auto        ToStringView() const { return pString ? std::string_view(pString) : std::string_view(); }

	operator	uintptr_t() const { return reinterpret_cast<uintptr_t>(pString); }
	operator	bool() const { return IsValid(); }

	bool		IsValid() const { return pString && *pString; }

	bool		operator <(CStringID Other) const noexcept { return pString < Other.pString; }
	bool		operator >(CStringID Other) const noexcept { return pString > Other.pString; }
	bool		operator <=(CStringID Other) const noexcept { return pString <= Other.pString; }
	bool		operator >=(CStringID Other) const noexcept { return pString >= Other.pString; }
	bool		operator ==(CStringID Other) const noexcept { return pString == Other.pString; }
	bool		operator !=(CStringID Other) const noexcept { return pString != Other.pString; }
	bool		operator ==(const char* pOther) const { return pString == pOther || (pString && pOther && !std::strcmp(pString, pOther)); }
	bool		operator !=(const char* pOther) const { return pString != pOther && (!pString || !pOther || std::strcmp(pString, pOther)); }
};

inline bool operator <(CStringID a, const char* b)
{
	return std::strcmp(a.CStr(), b) < 0;
}

inline bool operator <(const char* a, CStringID b)
{
	return std::strcmp(a, b.CStr()) < 0;
}

}

typedef Data::CStringID CStrID;

namespace std
{

// Hash raw pointer only, don't use string contents
template<>
struct hash<CStrID>
{
	size_t operator()(const CStrID _Keyval) const noexcept
	{
		return static_cast<size_t>(Hash(_Keyval.CStr()));
	}
};

}
