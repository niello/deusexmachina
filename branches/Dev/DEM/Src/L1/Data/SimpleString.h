#pragma once
#ifndef __DEM_L1_SIMPLE_STRING_H__
#define __DEM_L1_SIMPLE_STRING_H__

#include <string.h>
#include <StdDEM.h>
#include <Data/Hash.h>

// Simple static string, a wrapper around an LPCSTR

namespace Data
{

class CSimpleString
{
protected:

	char*	String;
	DWORD	Len;

	enum
	{
		FREE_MEM_THRESHOLD = 32
	};

public:

	CSimpleString(): String(NULL), Len(0) {}
	CSimpleString(LPCSTR Str): String(NULL), Len(0) { Set(Str); }
	CSimpleString(const CSimpleString& Str): String(NULL), Len(0) { Set((LPCSTR)Str); }
	~CSimpleString() { if (String) n_free(String); }

	void		Set(LPCSTR Str);
	LPCSTR		CStr() const { return String; }
	bool		IsValid() const { return String && *String; }
	DWORD		Length() const { return Len; }

	operator	DWORD() const { return (DWORD)String; }
	operator	LPCSTR() const { return String; }

	bool operator ==(const CSimpleString& Other) const { return Len == Other.Len && !strcmp(String, Other.String); }
	bool operator ==(LPCSTR Str) const { return !strcmp(String, Str); }
	bool operator !=(const CSimpleString& Other) const { return Len != Other.Len || strcmp(String, Other.String); }
	bool operator !=(LPCSTR Str) const { return !!strcmp(String, Str); }
	bool operator >(LPCSTR Str) const { return strcmp(String, Str) > 0; }
	bool operator <(LPCSTR Str) const { return strcmp(String, Str) < 0; }
	bool operator >=(LPCSTR Str) const { return strcmp(String, Str) >= 0; }
	bool operator <=(LPCSTR Str) const { return strcmp(String, Str) <= 0; }
	CSimpleString& operator =(const CSimpleString& Str) { Set((LPCSTR)Str); return *this; }
	CSimpleString& operator =(LPCSTR Str) { Set(Str); return *this; }
};

inline void CSimpleString::Set(LPCSTR Str)
{
	if (Str && *Str)
	{
		DWORD NewLen = strlen(Str);
		if (NewLen > Len || Len - NewLen > FREE_MEM_THRESHOLD)
			String = (char*)n_realloc(String, NewLen + 1);
		strcpy_s(String, NewLen + 1, Str);
		Len = NewLen;
	}
	else
	{
		if (String) n_free(String);
		String = NULL;
		Len = 0;
	}
}
//---------------------------------------------------------------------

}

template<> inline unsigned int Hash<Data::CSimpleString>(const Data::CSimpleString& Key)
{
	return Hash(Key.CStr(), Key.Length());
}
//---------------------------------------------------------------------

#endif
