#pragma once

// Compile-time function traits detection

namespace DEM::Meta
{

template<typename> struct function_traits;

template <typename Function>
struct function_traits : public function_traits<decltype(&std::remove_reference<Function>::type::operator())> {};

template <typename ClassType, typename ReturnType, typename... Arguments>
struct function_traits<ReturnType(ClassType::*)(Arguments...) const> : function_traits<ReturnType(*)(Arguments...)> {};

template <typename ClassType, typename ReturnType, typename... Arguments>
struct function_traits<ReturnType(ClassType::*)(Arguments...)> : function_traits<ReturnType(*)(Arguments...)> {};

template <typename ReturnType, typename... Arguments>
struct function_traits<ReturnType(*)(Arguments...)>
{
	typedef ReturnType result_type;

	template <std::size_t Index>
	using argument = typename std::tuple_element<Index, std::tuple<Arguments...>>::type;

	static const std::size_t arity = sizeof...(Arguments);
};

}
