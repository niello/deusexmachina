#pragma once
#ifndef __DEM_L1_SIMPLE_STRING_H__
#define __DEM_L1_SIMPLE_STRING_H__

#include <string.h>
#include <StdDEM.h>
#include <kernel/ntypes.h>

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

	operator	DWORD() const { return (DWORD)String; }
	operator	LPCSTR() const { return String; }

	bool operator ==(LPCSTR Str) const { return !strcmp(String, Str); }
	bool operator !=(LPCSTR Str) const { return !!strcmp(String, Str); }
	CSimpleString& operator =(LPCSTR Str) { Set(Str); return *this; }
	CSimpleString& operator =(const CSimpleString& Str) { Set((LPCSTR)Str); return *this; }
};
//---------------------------------------------------------------------

inline void CSimpleString::Set(LPCSTR Str)
{
	if (Str && *Str)
	{
		DWORD NewLen = strlen(Str);
		if (NewLen > Len || Len - NewLen > FREE_MEM_THRESHOLD)
			String = (char*)n_realloc(String, NewLen + 1);
		strcpy(String, Str);
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

#endif
