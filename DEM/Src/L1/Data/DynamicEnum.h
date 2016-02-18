#pragma once
#ifndef __DEM_L1_DYNAMIC_ENUM_H__
#define __DEM_L1_DYNAMIC_ENUM_H__

#include <StdDEM.h>
#include <Data/StringTokenizer.h>
#include <Data/Dictionary.h>

// Dynamic enum associates string names with bits. Use integer types as a template type.
// This class is designed for flag enums, where each value reserves a bit, and values can
// be logically combined. You can use aliases like All = X | Y | Z, as with regular enums.

namespace Data
{

template<class T>
class CDynamicEnumT
{
protected:

	CDict<CStrID, T>	Flags;
	UPTR				BitsUsed;

public:

	CDynamicEnumT(): BitsUsed(0) {}

	T		GetMask(const char* pFlagStr);
	void	SetAlias(CStrID Alias, const char* pFlagStr) { Flags.Add(Alias, GetMask(pFlagStr)); }
	void	SetAlias(CStrID Alias, T Mask) { Flags.Add(Alias, Mask); }
};

typedef CDynamicEnumT<U16> CDynamicEnum16;
typedef CDynamicEnumT<U32> CDynamicEnum32;
typedef CDynamicEnumT<UPTR> CDynamicEnum;

template<class T>
T CDynamicEnumT<T>::GetMask(const char* pFlagStr)
{
	T Mask = 0;

	Data::CStringTokenizer StrTok(pFlagStr);
	while (StrTok.GetNextToken("\t |") && *StrTok.GetCurrToken())
	{
		CStrID Flag = CStrID(StrTok.GetCurrToken());
		IPTR Idx = Flags.FindIndex(Flag);
		if (Idx != INVALID_INDEX) Mask |= Flags.ValueAt(Idx);
		else
		{
			if (BitsUsed >= (sizeof(T) << 3))
			{
				Sys::Error("CDynamicEnumT: overflow, flag '%s' would be %d-th", Flag.CStr(), BitsUsed + 1);
				return 0;
			}
			Mask |= Flags.Add(Flag, (1 << BitsUsed));
			++BitsUsed;
		}
	}
	return Mask;
}
//---------------------------------------------------------------------

}

#endif
