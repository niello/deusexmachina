#pragma once
#include <type_traits>

// Compile-time function traits detection

namespace DEM::Meta
{

template<typename> struct function_traits;

template<typename Function>
struct function_traits : public function_traits<decltype(&std::remove_reference<Function>::type::operator())> {};

template<typename ClassType, typename ReturnType, typename... Arguments>
struct function_traits<ReturnType(ClassType::*)(Arguments...) const> : function_traits<ReturnType(*)(Arguments...)> {};

template<typename ClassType, typename ReturnType, typename... Arguments>
struct function_traits<ReturnType(ClassType::*)(Arguments...)> : function_traits<ReturnType(*)(Arguments...)> {};

template<typename ReturnType, typename... Arguments>
struct function_traits<ReturnType(*)(Arguments...)>
{
	typedef ReturnType result_type;

	template<std::size_t Index>
	using argument = typename std::tuple_element<Index, std::tuple<Arguments...>>::type;

	static const std::size_t arity = sizeof...(Arguments);
};

#define HAS_METHOD_TRAIT(method_name) \
template<typename T> \
struct has_method_##method_name \
{ \
	template<typename U> static constexpr std::true_type test(decltype(&U::##method_name)); \
	template<typename> static constexpr std::false_type test(...); \
	static constexpr bool value = decltype(test<T>(0))::value; \
}; \
template<typename T> \
constexpr bool has_method_##method_name##_v = has_method_##method_name<T>::value;

#define HAS_METHOD_WITH_SIGNATURE_TRAIT(method_name) \
template<typename, typename> \
struct has_method_with_signature_##method_name { static constexpr bool value = std::false_type::value; }; \
template<typename T, typename Ret, typename... Args> \
struct has_method_with_signature_##method_name##<T, Ret(Args...)> \
{ \
	template<typename U> static constexpr auto test(U*) -> typename std::is_same<decltype(std::declval<U>().##method_name##(std::declval<Args>()...)), Ret>::type; \
	template<typename> static constexpr std::false_type test(...); \
	static constexpr bool value = decltype(test<T>(0))::value; \
}; \
template<typename T, typename Ret, typename... Args> \
constexpr bool has_method_with_signature_##method_name##_v = has_method_with_signature_##method_name##<T, Ret, Args...>::value;

}
