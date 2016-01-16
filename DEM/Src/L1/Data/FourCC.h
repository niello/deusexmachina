#pragma once
#ifndef __DEM_L1_FOURCC_H__
#define __DEM_L1_FOURCC_H__

#include <System/System.h>

// FourCC manipulation class

namespace Data
{

class CFourCC
{
public:

	U32 Code;

	CFourCC(): Code(0) {}
	CFourCC(I32 IntCode): Code(IntCode) { }
	CFourCC(U32 IntCode): Code(IntCode) { }
	CFourCC(const char* pString) { FromString(pString); }

	char		GetChar(U32 Idx) const { n_assert_dbg(Idx < 4); return (Code >> (Idx << 3)) & 0xff; } // Intel endianness
	void		FromString(const char* pString);
	void		ToString(char* Out) const;
	const char*	ToString() const;
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
inline void CFourCC::FromString(const char* pString)
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
inline const char* CFourCC::ToString() const
{
	static char GlobalFOURCC[5];
	ToString(GlobalFOURCC);
	return GlobalFOURCC;
}
//---------------------------------------------------------------------

}

#endif
