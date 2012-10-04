#pragma once
#ifndef __DEM_L1_STRING_ID_H__
#define __DEM_L1_STRING_ID_H__

#include <string.h>
#include <StdDEM.h>
#include <util/Hash.h>

// Static string identifier. The actual string is stored only once and all CStrIDs reference it
// by pointers. That is guaranteed that each string (case-sensitive) will have its unique address
// and all CStrIDs created from this string are the same.
// CStrIDs can be compared as integers, but still store informative string data inside.

namespace Data
{

class CStringID
{
protected:

	friend class CStrIDStorage;
	static class CStrIDStorage* Storage;

	LPCSTR String;

	explicit CStringID(LPCSTR string, int a, int b){ String = string; }

public:

	static const CStringID Empty;

	CStringID(): String(NULL) {}
#ifdef _DEBUG
	explicit // So I can later search all static StrIDs and predefine them
#endif
	CStringID(LPCSTR string, bool OnlyExisting = false);
	explicit	CStringID(void* StrID): String((LPCSTR)StrID) {} // Direct constructor. Be careful.
	explicit	CStringID(DWORD StrID): String((LPCSTR)StrID) {} // Direct constructor. Be careful.

	DWORD		GetID() const { return (DWORD)String; }
	LPCSTR		CStr() const { return String; }

	operator	DWORD() const { return (DWORD)String; }
	operator	LPCSTR() const { return String; }
	//operator	bool() const { return IsValid(); }

	bool		IsValid() const { return String && *String; }

	bool operator <(const CStringID& Other) const {return String<Other.String;}
	bool operator >(const CStringID& Other) const {return String>Other.String;}
	bool operator <=(const CStringID& Other) const {return String<=Other.String;}
	bool operator >=(const CStringID& Other) const {return String>=Other.String;}
	bool operator ==(const CStringID& Other) const {return String==Other.String;}
	bool operator !=(const CStringID& Other) const {return String!=Other.String;}
	bool operator ==(LPCSTR Str) const {return !strcmp(String, Str);}
	bool operator !=(LPCSTR Str) const {return strcmp(String, Str)!=0;}
	CStringID& operator =(const CStringID& Other) {String=Other.String; return *this;}
};

}

typedef Data::CStringID CStrID;

#endif
