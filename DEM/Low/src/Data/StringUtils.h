#pragma once
#include <Data/String.h>
#include <Data/StringID.h>
#include <Math/Matrix44.h>
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

inline CString	FromInt(I32 Value) { CString Str; Str.Format("%ld", Value); return Str; }
inline CString	FromUInt(U32 Value, bool Hex = false) { CString Str; Str.Format(Hex ? "%lx" : "%lu", Value); return Str; }
inline CString	FromFloat(float Value) { CString Str; Str.Format("%.6f", Value); return Str; }
inline CString	FromBool(bool Value) { return CString(Value ? "true" : "false"); }
inline CString	FromVector3(const vector3& v) { CString Str; Str.Format("%.6f,%.6f,%.6f", v.x, v.y, v.z); return Str; }
inline CString	FromVector4(const vector4& v) { CString Str; Str.Format("%.6f,%.6f,%.6f,%.6f", v.x, v.y, v.z, v.w); return Str; }
CString			FromMatrix44(const matrix44& m);

inline int		ToInt(const char* pStr) { return atoi(pStr); }
inline float	ToFloat(const char* pStr) { return (float)atof(pStr); }
bool			ToBool(const char* pStr);

char*			LastOccurrenceOf(const char* pStrStart, const char* pStrEnd, const char* pSubStr, UPTR SubStrLen = 0);
bool			MatchesPattern(const char* pStr, const char* pPattern);
//CString			Trim(const char* CharSet = DEM_WHITESPACE, bool Left = true, bool Right = true) const;

UPTR			StripComments(char* pStr, const char* pSingleLineComment = "//", const char* pMultiLineCommentStart = "/*", const char* pMultiLineCommentEnd = "*/"); 

inline bool AreEqualCaseInsensitive(const char* pStr1, const char* pStr2)
{
	return pStr1 == pStr2 || (pStr1 && pStr2 && !n_stricmp(pStr1, pStr2));
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

inline std::string ToString(const CString& Value)
{
	return std::string{ static_cast<std::string_view>(Value) };
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

inline std::string ToString(const matrix44& Value)
{
	return ToString(FromMatrix44(Value));
}
//---------------------------------------------------------------------

}
