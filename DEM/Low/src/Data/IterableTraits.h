#pragma once
#include <unordered_set>
#include <map>

// Helpers for iterability detection

namespace DEM::Meta
{

template<typename T>
struct is_pair : std::false_type {};

template<typename T, typename U>
struct is_pair<std::pair<T, U>> : std::true_type {};

template<typename T>
constexpr bool is_pair_v = is_pair<T>::value;

template<typename T, typename = void>
struct is_pair_iterable : std::false_type {};

template<typename T>
struct is_pair_iterable<T, std::enable_if_t<
	is_pair_v<typename std::iterator_traits<typename T::iterator>::value_type>
	>> : std::true_type {};

template<typename T>
constexpr bool is_pair_iterable_v = is_pair_iterable<T>::value;

template<typename T, typename = void>
struct is_single_iterable : std::false_type {};

template<typename T>
struct is_single_iterable<T, std::enable_if_t<
	!is_pair_v<typename std::iterator_traits<typename T::iterator>::value_type>
	>> : std::true_type {};

template<typename T>
constexpr bool is_single_iterable_v = is_single_iterable<T>::value;

template<typename T>
constexpr bool is_not_iterable_v = !is_single_iterable_v<T> && !is_pair_iterable_v<T>;

#define STD_SINGLE_CONTAINER_TRAIT(trait_type) \
template<typename T> \
struct is_std_##trait_type : std::false_type {}; \
template<typename T> \
struct is_std_##trait_type<std::trait_type<T>> : std::true_type {}; \
template<typename T> \
constexpr bool is_std_##trait_type##_v = is_std_##trait_type<T>::value;

#define STD_PAIR_CONTAINER_TRAIT(trait_type) \
template<typename T> \
struct is_std_##trait_type : std::false_type {}; \
template<typename K, typename V> \
struct is_std_##trait_type<std::trait_type<K, V>> : std::true_type {}; \
template<typename T> \
constexpr bool is_std_##trait_type##_v = is_std_##trait_type<T>::value;

STD_SINGLE_CONTAINER_TRAIT(vector);
STD_SINGLE_CONTAINER_TRAIT(set);
STD_SINGLE_CONTAINER_TRAIT(unordered_set);
STD_PAIR_CONTAINER_TRAIT(map);
STD_PAIR_CONTAINER_TRAIT(unordered_map);

/*
// https://stackoverflow.com/questions/13830158/check-if-a-variable-type-is-iterable
namespace detail
{
    // To allow ADL with custom begin/end
    using std::begin;
    using std::end;

    template <typename T>
    auto is_iterable_impl(int)
    -> decltype (
        begin(std::declval<T&>()) != end(std::declval<T&>()), // begin/end and operator !=
        void(), // Handle evil operator ,
        ++std::declval<decltype(begin(std::declval<T&>()))&>(), // operator ++
        void(*begin(std::declval<T&>())), // operator*
        std::true_type{});

    template <typename T>
    std::false_type is_iterable_impl(...);

}

template <typename T>
using is_iterable = decltype(detail::is_iterable_impl<T>(0));
*/

}
