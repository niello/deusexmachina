#pragma once
#ifndef __DEM_L1_FOURCC_H__
#define __DEM_L1_FOURCC_H__

#include <StdDEM.h>

// FourCC manipulation class

namespace Data
{

class CFourCC
{
public:

	DWORD Code;

	CFourCC(): Code(0) {}
	CFourCC(int IntCode): Code(IntCode) { }
	CFourCC(DWORD IntCode): Code(IntCode) { }
	CFourCC(LPCSTR pString) { FromString(pString); }

	char		GetChar(DWORD Idx) const { n_assert_dbg(Idx < 4); return (Code >> (Idx << 3)) & 0xff; } // Intel endianness
	void		FromString(LPCSTR pString);
	void		ToString(char* Out) const;
	LPCSTR		ToString() const;
	bool		IsValid() const { return !!Code; }

	CFourCC&	operator =(const CFourCC& Other) { Code = Other.Code; return *this; }
	bool		operator ==(const CFourCC& Other) const { return Code == Other.Code; }
	bool		operator !=(const CFourCC& Other) const { return Code != Other.Code; }
	bool		operator <(const CFourCC& Other) const { return Code < Other.Code; }
	bool		operator >(const CFourCC& Other) const { return Code > Other.Code; }
	bool		operator <=(const CFourCC& Other) const { return Code <= Other.Code; }
	bool		operator >=(const CFourCC& Other) const { return Code >= Other.Code; }
};
//---------------------------------------------------------------------

// Convert "ABCD" as compiler converts 'ABCD' character constant
inline void CFourCC::FromString(LPCSTR pString)
{
	n_assert_dbg(pString);
	Code = pString[3] | (pString[2] << 8) | (pString[1] << 16) | (pString[0] << 24);
}
//---------------------------------------------------------------------

inline void CFourCC::ToString(char* Out) const
{
	Out[0] = (Code >> 24) & 0xff;
	Out[1] = (Code >> 16) & 0xff;
	Out[2] = (Code >> 8) & 0xff;
	Out[3] = Code & 0xff;
	Out[4] = 0;
}
//---------------------------------------------------------------------

// Thread-unsafe
inline LPCSTR CFourCC::ToString() const
{
	static char GlobalFOURCC[5];
	ToString(GlobalFOURCC);
	return GlobalFOURCC;
}
//---------------------------------------------------------------------

}

#endif
