#pragma once
#include <Data/StringUtils.h>
#include <magic_enum/magic_enum_format.hpp>

// Enum manipulation utilities

using namespace magic_enum::bitwise_operators;

template <typename T, typename std::enable_if_t<std::is_enum_v<T>>* = nullptr>
std::optional<T> EnumFromString(std::string_view Value)
{
	std::optional<T> Result;
	StringUtils::Tokenize(Value, '|', [&Result](std::string_view id)
	{
		const auto Curr = magic_enum::enum_cast<T>(id);
		if (!Curr)
		{
			Result = std::nullopt;
			return true; // break on failure
		}

		Result = Result ? (*Result | *Curr) : Curr;
		return false; // continue
	});

	return Result;
}
//---------------------------------------------------------------------

namespace StringUtils
{

template <typename T, typename std::enable_if_t<std::is_enum_v<std::decay_t<T>>>* = nullptr>
inline std::string ToString(T Value)
{
	return magic_enum::detail::format_as<T>(Value);
}
//---------------------------------------------------------------------

}
