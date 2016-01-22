#pragma once
#ifndef __DEM_L1_STRING_ID_H__
#define __DEM_L1_STRING_ID_H__

#include <StdDEM.h>
#include <Data/Hash.h>

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

	const char* String;

	explicit CStringID(const char* pString, int, int) { String = pString; }

public:

	static const CStringID Empty;

	CStringID(): String(NULL) {}
#ifdef _DEBUG
	explicit // So I can later search all static StrIDs and predefine them
#endif
	CStringID(const char* pString, bool OnlyExisting = false);
	explicit CStringID(void* StrID): String((const char*)StrID) {} // Direct constructor. Be careful.
	explicit CStringID(UPTR StrID): String((const char*)StrID) {} // Direct constructor. Be careful.

	UPTR		GetID() const { return (UPTR)String; }
	const char*	CStr() const { return String; }

	operator	UPTR() const { return (UPTR)String; }
	operator	const char*() const { return String; }
	//operator	bool() const { return IsValid(); }

	bool		IsValid() const { return String && *String; }

	bool		operator <(const CStringID& Other) const { return String < Other.String; }
	bool		operator >(const CStringID& Other) const { return String > Other.String; }
	bool		operator <=(const CStringID& Other) const { return String <= Other.String; }
	bool		operator >=(const CStringID& Other) const { return String >= Other.String; }
	bool		operator ==(const CStringID& Other) const { return String == Other.String; }
	bool		operator !=(const CStringID& Other) const { return String != Other.String; }
	bool		operator ==(const char* Str) const { return !strcmp(String, Str); }
	bool		operator !=(const char* Str) const { return !!strcmp(String, Str); }
	CStringID&	operator =(const CStringID& Other) { String = Other.String; return *this; }
};

}

typedef Data::CStringID CStrID;

#endif
