#pragma once
#include <Data/SerializeToParams.h>
#include <magic_enum/magic_enum_format.hpp>

// Enum manipulation utilities

namespace DEM::Serialization
{

template<typename T>
struct ParamsFormat<T, typename std::enable_if_t<std::is_enum_v<std::decay_t<T>>>>
{
	static inline void Serialize(Data::CData& Output, T Value)
	{
		Output = std::string{ magic_enum::enum_name(Value) };
	}

	static inline void Deserialize(const Data::CData& Input, T& Value)
	{
		if (auto* pInput = Input.As<std::string>())
		{
			if (auto EnumOpt = magic_enum::enum_cast<std::decay_t<T>>(*pInput))
				Value = EnumOpt.value();
		}
		else if (auto* pInput = Input.As<int>())
		{
			if (auto EnumOpt = magic_enum::enum_cast<std::decay_t<T>>(*pInput))
				Value = EnumOpt.value();
		}
	}
};

}

namespace StringUtils
{

template <typename T>
inline std::enable_if_t<std::is_enum_v<std::decay_t<T>>, std::string> ToString(T Value)
{
	return magic_enum::detail::format_as<T>(Value);
}
//---------------------------------------------------------------------

}
