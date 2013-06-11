#pragma once
#ifndef __DEM_L1_DYNAMIC_ENUM_H__
#define __DEM_L1_DYNAMIC_ENUM_H__

#include <StdDEM.h>

// Dynamic enum associates string names with bits. Use integer types as a template type.
// This class is designed for flag enums, where each value reserves a bit, and values can
// be logically combined. You can use aliases like All = X | Y | Z, like in regular enums.

namespace Data
{

template<class T>
class CDynamicEnumT
{
protected:

	nDictionary<CStrID, T>	Flags;
	DWORD					BitsUsed;

public:

	CDynamicEnumT(): BitsUsed(0) {}

	T		GetMask(const nString& FlagStr);
	void	SetAlias(CStrID Alias, const nString& FlagStr) { Flags.Add(Alias, GetMask(FlagStr)); }
	void	SetAlias(CStrID Alias, T Mask) { Flags.Add(Alias, Mask); }
};

typedef CDynamicEnumT<ushort> CDynamicEnum16;
typedef CDynamicEnumT<DWORD> CDynamicEnum32;

template<class T>
T CDynamicEnumT<T>::GetMask(const nString& FlagStr)
{
	T Mask = 0;

	nArray<nString> FlagsToFind;
	FlagStr.Tokenize("\t |", FlagsToFind);
	for (int i = 0; i < FlagsToFind.GetCount(); ++i)
	{
		CStrID Flag = CStrID(FlagsToFind[i].CStr());
		int Idx = Flags.FindIndex(Flag);
		if (Idx != INVALID_INDEX) Mask |= Flags.ValueAtIndex(Idx);
		else
		{
			if (BitsUsed >= sizeof(T) * 8)
			{
				n_error("CDynamicEnumT: overflow, flag %s would be %d-th", Flag.CStr(), BitsUsed + 1);
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
