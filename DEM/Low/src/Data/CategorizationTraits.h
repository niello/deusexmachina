#pragma once
#include <unordered_set>
#include <map>

// Helpers for compile-time type categorization

namespace DEM::Meta
{

template<class T, class R = void>
struct enable_if_type { typedef R type; };

#define META_DECLARE_TYPEDEF_FLAG(TAG) \
template<class T, class Enable = void> \
struct is_typedef_flag_##TAG : std::false_type {}; \
template<class T> \
struct is_typedef_flag_##TAG<T, typename enable_if_type<typename T::TAG>::type> : T::TAG {}; \
template<typename T> \
constexpr bool is_typedef_flag_##TAG##_v = is_typedef_flag_##TAG<T>::value;

#define META_DECLARE_BOOL_FLAG(TAG) \
template <typename T, typename = int> \
struct is_bool_flag_##TAG : std::false_type {}; \
template <typename T> \
struct is_bool_flag_##TAG<T, decltype(T::TAG, 0)> { constexpr static bool value = static_cast<bool>(T::TAG); }; \
template<typename T> \
constexpr bool is_bool_flag_##TAG##_v = is_bool_flag_##TAG<T>::value;

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

template<typename T>
constexpr bool is_single_collection_v = is_single_iterable_v<T> && !std::is_same_v<T, std::string>;

template<typename T>
constexpr bool is_not_collection_v = is_not_iterable_v<T> || std::is_same_v<T, std::string>;

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
