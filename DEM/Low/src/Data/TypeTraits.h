#pragma once
#include <type_traits>
#include <utility>

// Various compile-time type traits and utilities

namespace DEM::Meta
{
// TODO: C++20 std::equality_comparable, std::equality_comparable_with
template <class T, class U, class = void>
constexpr bool is_equality_comparable = false;

template <class T, class U>
constexpr bool is_equality_comparable<T, U, std::void_t<decltype(std::declval<T&>() == std::declval<U&>())>> = true;

// https://stackoverflow.com/questions/27338428/variadic-template-that-determines-the-best-conversion
// NB: yields 'void' for ambiguous conversions. Numeric conversions are frequently ambiguous, e.g. integral vs float.
template <typename T, typename... E>
struct best_conversion
{
	template <typename...> struct overloads {};

	template <typename U, typename... Rest>
	struct overloads<U, Rest...> : overloads<Rest...>
	{
		using overloads<Rest...>::call;
		static U call(U);
	};

	template <typename U>
	struct overloads<U>
	{
		static U call(U);
	};

	template <typename... E_>
	static decltype(overloads<E_...>::call(std::declval<T>())) best_conv(int);

	template <typename...>
	static void best_conv(...);

	using type = decltype(best_conv<E...>(0));
};

template <typename... T>
using best_conversion_t = typename best_conversion<T...>::type;
//---------------------------------------------------------------------

template<typename T, typename... TTypes>
constexpr size_t index_of_type()
{
	size_t i = 0;
	const bool Found = ((++i && std::is_same_v<T, TTypes>) || ...);
	return i - Found;
}
//---------------------------------------------------------------------

template<typename T, typename... TTypes>
constexpr bool contains_type()
{
	return (std::is_same_v<T, TTypes> || ...);
}
//---------------------------------------------------------------------

// https://stackoverflow.com/questions/46278997/variadic-templates-and-switch-statement
template<typename T, T... Is, typename F>
decltype(auto) compile_switch(T i, std::integer_sequence<T, Is...>, F f)
{
	using return_type = std::common_type_t<decltype(f(std::integral_constant<T, Is>{}))... > ;
	if constexpr (std::is_void_v<return_type>)
	{
		std::initializer_list<int>({ (i == Is ? (f(std::integral_constant<T, Is>{})),0 : 0)... });
	}
	else
	{
		return_type ret{};
		std::initializer_list<int>({ (i == Is ? (ret = f(std::integral_constant<T, Is>{})),0 : 0)... });
		return ret;
	}
}
//---------------------------------------------------------------------

}
