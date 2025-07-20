#pragma once
#include <Data/StringID.h>
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

	T GetMask(std::string_view FlagStr);
	T SetAlias(CStrID Alias, std::string_view FlagStr) { return Flags.emplace(Alias, GetMask(FlagStr)).first->second; }
	T SetAlias(CStrID Alias, T Mask) { return Flags.emplace(Alias, Mask).first->second; }
};

template<class T>
T CDynamicEnumT<T>::GetMask(std::string_view FlagStr)
{
	T Mask = 0;

	size_t Start = 0;
	while (Start < FlagStr.size())
	{
		const size_t End = FlagStr.find('|', Start);
		const CStrID Flag = CStrID(FlagStr.substr(Start, End - Start));

		auto It = Flags.find(Flag);
		if (It != Flags.cend())
		{
			Mask |= It->second;
		}
		else
		{
			if (BitsUsed >= (sizeof(T) << 3))
			{
				::Sys::Error("CDynamicEnumT<T>::GetMask() > overflow, flag '{}' would be {}-th"_format(Flag, BitsUsed + 1));
				return 0;
			}

			const auto BitValue = (1 << BitsUsed);
			Flags.emplace(Flag, BitValue);
			++BitsUsed;

			Mask |= BitValue;
		}

		if (End == std::string_view::npos) break;

		Start = End + 1;
	}

	return Mask;
}
//---------------------------------------------------------------------

using CDynamicEnum8 = CDynamicEnumT<U8>;
using CDynamicEnum16 = CDynamicEnumT<U16>;
using CDynamicEnum32 = CDynamicEnumT<U32>;
using CDynamicEnum64 = CDynamicEnumT<U64>;
using CDynamicEnum = CDynamicEnumT<UPTR>;

}
