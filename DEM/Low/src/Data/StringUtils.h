#pragma once
#include <Data/StringID.h>
#include <Math/Matrix44.h>
#include <rtm/vector4f.h>
#include <string>
#include <optional>
#include <variant>

// String manipulation and utility functions

namespace StringUtils
{

// A wrapper which combines overwriting and back-insertion
class CStringAppender
{
public:

	CStringAppender(std::string& Str, size_t Pos = 0) : _Str(Str), _Pos(Pos) {}

	void Append(std::string_view Data)
	{
		_Str.erase(_Pos);
		_Str.append(Data);
		_Pos = _Str.size();
	}

protected:

	std::string& _Str;
	size_t       _Pos = 0;
};

inline int		ToInt(const char* pStr) { return atoi(pStr); }
inline float	ToFloat(const char* pStr) { return (float)atof(pStr); }
bool			ToBool(const char* pStr);

char*			LastOccurrenceOf(const char* pStrStart, const char* pStrEnd, const char* pSubStr, UPTR SubStrLen = 0);
bool			MatchesPattern(const char* pStr, const char* pPattern);

UPTR			StripComments(char* pStr, const char* pSingleLineComment = "//", const char* pMultiLineCommentStart = "/*", const char* pMultiLineCommentEnd = "*/"); 

inline bool AreEqualCaseInsensitive(const char* pStr1, const char* pStr2)
{
	return pStr1 == pStr2 || (pStr1 && pStr2 && !n_stricmp(pStr1, pStr2));
}
//---------------------------------------------------------------------

template<typename F, typename D>
void Tokenize(std::string_view Input, D Dlm, F Callback)
{
	size_t Start = 0;
	size_t End;
	while ((End = Input.find(Dlm, Start)) != std::string_view::npos)
	{
		if constexpr (std::is_invocable_r_v<bool, F, std::string_view>)
		{
			if (Callback(Input.substr(Start, End - Start))) return;
		}
		else if constexpr (std::is_invocable_r_v<void, F, std::string_view>)
		{
			Callback(Input.substr(Start, End - Start));
		}
		Start = End + 1;
	}

	// Handle the last token
	if (Start < Input.size())
	{
		if constexpr (std::is_invocable_r_v<bool, F, std::string_view>)
		{
			if (Callback(Input.substr(Start))) return;
		}
		else if constexpr (std::is_invocable_r_v<void, F, std::string_view>)
		{
			Callback(Input.substr(Start));
		}
	}
}
//---------------------------------------------------------------------

inline std::string_view TrimLeft(std::string_view Input, std::string_view Chars = DEM_WHITESPACE)
{
	const size_t First = Input.find_first_not_of(Chars);
	return (First == std::string_view::npos) ? ""sv : Input.substr(First);
}
//---------------------------------------------------------------------

inline std::string_view TrimRight(std::string_view Input, std::string_view Chars = DEM_WHITESPACE)
{
	const size_t Last = Input.find_last_not_of(Chars);
	return (Last == std::string_view::npos) ? ""sv : Input.substr(0, Last + 1);
}
//---------------------------------------------------------------------

inline std::string_view Trim(std::string_view Input, std::string_view Chars = DEM_WHITESPACE)
{
	return TrimLeft(TrimRight(Input, Chars), Chars);
}
//---------------------------------------------------------------------

// NB: modifies input string
inline std::string& TrimTrailingDecimalZeros(std::string& Str)
{
	const auto DotPos = Str.find_last_of('.');
	if (DotPos != std::string::npos)
	{
		const auto NonZeroPos = Str.find_last_not_of('0', DotPos + 1);
		if (NonZeroPos + 1 < Str.size())
			Str.erase(NonZeroPos + 1);
	}

	return Str;
}
//---------------------------------------------------------------------

// *** ToString overloads ***
// TODO: use {fmt}

inline const std::string& ToString(const std::string& Value)
{
	return Value;
}
//---------------------------------------------------------------------

inline std::string ToString(std::string_view Value)
{
	return std::string{ Value };
}
//---------------------------------------------------------------------

inline std::string ToString(const char* Value)
{
	return Value ? std::string{ Value } : std::string{};
}
//---------------------------------------------------------------------

inline std::string ToString(CStrID Value)
{
	return Value.ToString();
}
//---------------------------------------------------------------------

inline std::string ToString(bool Value)
{
	return Value ? "true" : "false";
}
//---------------------------------------------------------------------

// Integers
template <typename T>
inline std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, std::string> ToString(T Value)
{
	return std::to_string(Value);
}
//---------------------------------------------------------------------

// Floats
template <typename T>
inline std::enable_if_t<std::is_floating_point_v<T>, std::string> ToString(T Value)
{
	return std::to_string(Value); // TODO: with {fmt} precision will be possible!
}
//---------------------------------------------------------------------

// Optionals
template <typename T>
inline std::string ToString(const std::optional<T>& Value, std::string_view NullOpt = {})
{
	return Value ? ToString(*Value) : std::string{ NullOpt };
}
//---------------------------------------------------------------------

// Variants

inline std::string ToString(std::monostate Value)
{
	return std::string{};
}
//---------------------------------------------------------------------

template <typename... TArgs>
inline std::string ToString(const std::variant<TArgs...>& Value)
{
	return std::visit([](auto&& SubValue) { return ToString(SubValue); }, Value);
}
//---------------------------------------------------------------------

// Math
// FIXME: very inefficient, use {fmt}!

inline std::string ToString(const vector3& Value)
{
	return ToString(Value.x) + ',' + ToString(Value.y) + ',' + ToString(Value.z);
}
//---------------------------------------------------------------------

inline std::string ToString(const vector4& Value)
{
	return ToString(Value.x) + ',' + ToString(Value.y) + ',' + ToString(Value.z) + ',' + ToString(Value.w);
}
//---------------------------------------------------------------------

inline std::string ToString(rtm::vector4f_arg0 Value)
{
	const float x = rtm::vector_get_x(Value);
	const float y = rtm::vector_get_y(Value);
	const float z = rtm::vector_get_z(Value);
	const float w = rtm::vector_get_w(Value);
	return ToString(x) + ',' + ToString(y) + ',' + ToString(z) + ',' + ToString(w);
}
//---------------------------------------------------------------------

inline std::string ToString(const matrix44& m)
{
	return
		"{:.6f}, {:.6f}, {:.6f}, {:.6f}, "
		"{:.6f}, {:.6f}, {:.6f}, {:.6f}, "
		"{:.6f}, {:.6f}, {:.6f}, {:.6f}, "
		"{:.6f}, {:.6f}, {:.6f}, {:.6f}"_format(
			m.M11, m.M12, m.M13, m.M14,
			m.M21, m.M22, m.M23, m.M24,
			m.M31, m.M32, m.M33, m.M34,
			m.M41, m.M42, m.M43, m.M44);
}
//---------------------------------------------------------------------

}
