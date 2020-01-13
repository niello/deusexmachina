#pragma once
#include <Data/StringID.h>
#include <Data/StringTokenizer.h>
#include <map>

// Dynamic enum associates string names with bits. Use integer types as a template type.
// This class is designed for flag enums, where each value reserves a bit, and values can
// be logically combined. You can use aliases like All = X | Y | Z, as with regular enums.

namespace Data
{

template<class T>
class CDynamicEnumT
{
protected:

	std::map<CStrID, T> Flags;
	U8                  BitsUsed = 0;

public:

	T		GetMask(const char* pFlagStr);
	void	SetAlias(CStrID Alias, const char* pFlagStr) { Flags.emplace(Alias, GetMask(pFlagStr)); }
	void	SetAlias(CStrID Alias, T Mask) { Flags.emplace(Alias, Mask); }
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

		auto It = Flags.find(Flag);
		if (It != Flags.cend())
		{
			Mask |= It->second;
		}
		else
		{
			if (BitsUsed >= (sizeof(T) << 3))
			{
				::Sys::Error("CDynamicEnumT: overflow, flag '%s' would be %d-th", Flag.CStr(), BitsUsed + 1);
				return 0;
			}

			const auto BitValue = (1 << BitsUsed);
			Flags.emplace(Flag, BitValue);
			++BitsUsed;

			Mask |= BitValue;
		}
	}

	return Mask;
}
//---------------------------------------------------------------------

}
