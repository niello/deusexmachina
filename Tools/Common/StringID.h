#pragma once
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
	static class CStringIDStorage Storage;

	const char* pString = nullptr;

	explicit CStringID(const char* pStr, int, int) { pString = pStr; }

public:

	static const CStringID Empty;

	CStringID() {}
	CStringID(const CStringID& Other) : pString(Other.pString) {}
	CStringID(CStringID&& Other) : pString(Other.pString) {}
#ifdef _DEBUG
	explicit // So I can later search all static StrIDs and predefine them
#endif
	CStringID(const char* pStr, bool OnlyExisting = false);
	explicit CStringID(const std::string& Str): CStringID(Str.c_str()) {}
	explicit CStringID(void* StrID): pString((const char*)StrID) {} // Direct constructor. Be careful.
	explicit CStringID(size_t StrID): pString((const char*)StrID) {} // Direct constructor. Be careful.

	size_t		GetID() const { return reinterpret_cast<size_t>(pString); }
	const char*	CStr() const { return pString; }
	std::string ToString() const { return pString ? std::string(pString) : std::string(); }

	operator	size_t() const { return reinterpret_cast<size_t>(pString); }
	operator	const char*() const { return pString; }
	//operator	bool() const { return IsValid(); }

	bool		IsValid() const { return pString && *pString; }

	bool		operator <(const CStringID& Other) const { return pString < Other.pString; }
	bool		operator >(const CStringID& Other) const { return pString > Other.pString; }
	bool		operator <=(const CStringID& Other) const { return pString <= Other.pString; }
	bool		operator >=(const CStringID& Other) const { return pString >= Other.pString; }
	bool		operator ==(const CStringID& Other) const { return pString == Other.pString; }
	bool		operator !=(const CStringID& Other) const { return pString != Other.pString; }
	bool		operator ==(const char* pOther) const { return pString == pOther || (pString && pOther && !strcmp(pString, pOther)); }
	bool		operator !=(const char* pOther) const { return pString != pOther && (!pString || !pOther || strcmp(pString, pOther)); }
	CStringID&	operator =(const CStringID& Other) { pString = Other.pString; return *this; }
	CStringID&	operator =(CStringID&& Other) { pString = Other.pString; return *this; }
};

}

typedef Data::CStringID CStrID;
