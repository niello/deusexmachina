#pragma once
#include <StdDEM.h>
#include <System/System.h>
#include <map>

// Fixed size two-dimensional array.

//!!!TODO: move to appropriate header!
namespace DEM::Meta
{

template<class T, class... TTypes>
constexpr size_t index_of_type()
{
	size_t i = 0;
	const bool Found = ((++i && std::is_same_v<T, TTypes>) || ...);
	return i - Found;
}

template<class T, class... TTypes>
constexpr bool contains_type()
{
	return (std::is_same_v<T, TTypes> || ...);
}

}

//???make struct?! to ensure external transparency and strict type safety without implicit conversions.
using HVar = uint32_t; // 4 bits of type index and 28 bits of index in a corresponding vector
constexpr HVar InvalidVar = { 0xffffffff };
constexpr size_t VAR_INDEX_BITS = 28;

template<typename... TVarTypes>
class CVarStorage
{
protected:

	template<typename T>
	using pass = std::conditional_t<(sizeof(T) > sizeof(size_t)), const T&, T>;

	template<typename T>
	bool IsTypeValid(HVar Handle) const
	{
		return (Handle >> VAR_INDEX_BITS) == DEM::Meta::index_of_type<T, TVarTypes...>();
	}

	std::tuple<std::vector<TVarTypes>...> _Storages;
	std::map<CStrID, HVar>                _VarsByID;

public:

	template<typename T>
	pass<T> Get(HVar Handle) const
	{
		// Explicit check saves us from a spam of compiler errors with teh same meaning from std::get
		static_assert(DEM::Meta::contains_type<T, TVarTypes...>(), "Requested type is not supported by this storage");
		n_assert_dbg(IsTypeValid<T>(Handle));
		return std::get<std::vector<T>>(_Storages)[Handle & ((1 << VAR_INDEX_BITS) - 1)];
	}

	template<typename T>
	void Set(HVar Handle, pass<T> Value)
	{
		n_assert_dbg(IsTypeValid<T>(Handle));
		if (IsTypeValid<T>(Handle))
			std::get<std::vector<T>>(_Storages)[Handle & ((1 << VAR_INDEX_BITS) - 1)] = Value;
	}

	template<typename T, typename std::enable_if_t<!std::is_same_v<pass<T>, T>>* = nullptr>
	void Set(HVar Handle, T&& Value)
	{
		n_assert_dbg(IsTypeValid<T>(Handle));
		if (IsTypeValid<T>(Handle))
			std::get<std::vector<T>>(_Storages)[Handle & ((1 << VAR_INDEX_BITS) - 1)] = std::move(Value);
	}

	template<typename T>
	HVar Set(CStrID ID, pass<T> Value)
	{
		//???can also check a convertible type if not found?! for this will need index_of_convertible_type!
		HVar Handle = InvalidVar;
		Set<T>(Handle, Value);
		return Handle;
	}

	template<typename T, typename std::enable_if_t<!std::is_same_v<pass<T>, T>>* = nullptr>
	HVar Set(CStrID ID, T&& Value)
	{
		//???can also check a convertible type if not found?! for this will need index_of_convertible_type!
		HVar Handle = InvalidVar;
		Set<T>(Handle, std::move(Value));
		return Handle;
	}
};
