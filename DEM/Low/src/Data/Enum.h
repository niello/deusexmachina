#pragma once
//#include <Data/StringUtils.h>
#include <magic_enum/magic_enum_format.hpp>

// Enum manipulation utilities

using namespace magic_enum::bitwise_operators;

//???TODO: move ENUM_MASK here?

template<typename T, typename std::enable_if_t<std::is_enum_v<T>>* = nullptr>
class CEnumMask
{
protected:

	// Relies on the fact that enum_values are sorted ascending
	static constexpr auto MaxValue = static_cast<size_t>(magic_enum::enum_values<T>().back());
	static_assert(MaxValue < 64, "Can't make a enum mask type for more than 64 bit flags");

	using TMask =
		std::conditional_t < MaxValue < 8, uint8_t,
		std::conditional_t < MaxValue < 16, uint16_t,
		std::conditional_t<MaxValue < 32, uint32_t,
		uint64_t>>>;

	TMask Mask = 0;

public:

	constexpr CEnumMask() = default;
	constexpr explicit CEnumMask(TMask Mask) : _Mask(Mask) {}

	template<typename... TArgs, typename = std::enable_if_t<(std::is_same_v<T, TArgs> && ...)>>
	constexpr CEnumMask(TArgs... Values) : _Mask((... | (static_cast<TMask>(1) << static_cast<TMask>(Values)))) {}

	void Add(T Flag) { _Mask |= (static_cast<TMask>(1) << static_cast<TMask>(Flag)); }
	void Remove(T Flag) { _Mask &= ~(static_cast<TMask>(1) << static_cast<TMask>(Flag)); }
	constexpr bool Has(T Flag) const noexcept { return (_Mask & (static_cast<TMask>(1) << static_cast<TMask>(Flag))) != 0; }

	explicit constexpr operator bool() const noexcept { return !!_Mask; }
	explicit constexpr operator TMask() const noexcept { return _Mask; }

	constexpr bool operator ==(CEnumMask Other) const noexcept { return _Mask == Other._Mask; }
	constexpr bool operator !=(CEnumMask Other) const noexcept { return _Mask != Other._Mask; }

	constexpr CEnumMask& operator |=(T Flag) noexcept { Add(Flag); return *this; }
	constexpr CEnumMask& operator |=(CEnumMask Other) noexcept { _Mask |= Other._Mask; return *this; }
	constexpr CEnumMask& operator &=(CEnumMask Other) noexcept { _Mask &= Other._Mask; return *this; }
	constexpr CEnumMask& operator ^=(CEnumMask Other) noexcept { _Mask ^= Other._Mask; return *this; }

	friend constexpr CEnumMask operator |(CEnumMask Lhs, T Rhs) noexcept { return Lhs | CEnumMask(Rhs); }
	friend constexpr CEnumMask operator |(T Lhs, CEnumMask Rhs) noexcept { return CEnumMask(Lhs) | Rhs; }
	friend constexpr CEnumMask operator &(CEnumMask Lhs, T Rhs) noexcept { return Lhs & CEnumMask(Rhs); }
	friend constexpr CEnumMask operator &(T Lhs, CEnumMask Rhs) noexcept { return CEnumMask(Lhs) & Rhs; }
	friend constexpr CEnumMask operator ^(CEnumMask Lhs, T Rhs) noexcept { return Lhs ^ CEnumMask(Rhs); }
	friend constexpr CEnumMask operator ^(T Lhs, CEnumMask Rhs) noexcept { return CEnumMask(Lhs) ^ Rhs; }

	friend constexpr CEnumMask operator |(CEnumMask Lhs, CEnumMask Rhs) noexcept { return CEnumMask(Lhs._Mask | Rhs._Mask); }
	friend constexpr CEnumMask operator &(CEnumMask Lhs, CEnumMask Rhs) noexcept { return CEnumMask(Lhs._Mask & Rhs._Mask); }
	friend constexpr CEnumMask operator ^(CEnumMask Lhs, CEnumMask Rhs) noexcept { return CEnumMask(Lhs._Mask ^ Rhs._Mask); }
	friend constexpr CEnumMask operator ~(CEnumMask Lhs) noexcept { return CEnumMask(~Lhs._Mask); }
};

template<typename T, typename... TRest>
CEnumMask(T, TRest...) -> CEnumMask<T>;

template<typename T, typename std::enable_if_t<std::is_enum_v<T>>* = nullptr>
std::optional<T> EnumFromString(std::string_view Value)
{
	//std::optional<T> Result;
	//StringUtils::Tokenize(Value, '|', [&Result](std::string_view Part)
	//{
	//	if (const auto CurrOpt = magic_enum::enum_cast<T>(Part))
	//	{
	//		Result = Result ? (*Result | *CurrOpt) : CurrOpt;
	//		return false; // continue
	//	}

	//	Result = std::nullopt;
	//	return true; // break on failure
	//});

	//return Result;

	//!!!doesn't work if enum contains zero! can't use for regular enums! only for flags!
	//???if not enum_flags_cast, try enum_cast? or check details::has_is_flags?
	return magic_enum::enum_flags_cast<T>(Value);
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
