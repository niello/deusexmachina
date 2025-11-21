#pragma once
#include <magic_enum/magic_enum_format.hpp>

// Enum manipulation utilities

namespace StringUtils
{

// TODO: or move this to StringUtils.h and don't use Enum.h at all?
template <typename T>
inline std::enable_if_t<std::is_enum_v<std::decay_t<T>>, std::string> ToString(T Value)
{
	return magic_enum::detail::format_as<T>(Value);
}
//---------------------------------------------------------------------

}
