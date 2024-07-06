#pragma once
#include <Data/Params.h>
#include <Data/DataArray.h>
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
constexpr size_t index_of_convertible_type()
{
	size_t i = 0;
	const bool Found = ((++i && std::is_convertible_v<T, TTypes>) || ...);
	return i - Found;
}

template<class T, class... TTypes>
constexpr bool contains_type()
{
	return (std::is_same_v<T, TTypes> || ...);
}

// https://stackoverflow.com/questions/46278997/variadic-templates-and-switch-statement
template <class T, T... Is, class F>
auto compile_switch(T i, std::integer_sequence<T, Is...>, F f)
{
	using return_type = std::common_type_t<decltype(f(std::integral_constant<T, Is>{}))...>;
	return_type ret{};
	std::initializer_list<int>({ (i == Is ? (ret = f(std::integral_constant<T, Is>{})),0 : 0)... });
	return ret;
}

}

//???make struct?! to ensure external transparency and strict type safety without implicit conversions.
using HVar = uint32_t; // 4 bits of type index and 28 bits of index in a corresponding vector
constexpr HVar InvalidVar = { 0xffffffff };
constexpr size_t VAR_INDEX_BITS = 28;
constexpr size_t VAR_TYPE_INDEX_BITS = sizeof(HVar) * 8 - VAR_INDEX_BITS;

template<typename... TVarTypes>
class CVarStorage
{
protected:

	static_assert(sizeof...(TVarTypes) < (1 << VAR_TYPE_INDEX_BITS), "Too many types to be indexed in HVar"); // NB: one value is reserved for invalid index

	template<typename T>
	using pass = std::conditional_t<(sizeof(T) > sizeof(size_t)), const T&, T>;

	template<typename T>
	static constexpr auto TypeIndex = DEM::Meta::contains_type<T, TVarTypes...>() ?
		DEM::Meta::index_of_type<T, TVarTypes...>() :
		DEM::Meta::index_of_convertible_type<T, TVarTypes...>();

	std::tuple<std::vector<TVarTypes>...> _Storages;
	std::map<CStrID, HVar>                _VarsByID;

	template<typename T>
	bool IsTypeValid(HVar Handle) const
	{
		return (Handle >> VAR_INDEX_BITS) == TypeIndex<T>;
	}

	template<typename T>
	bool TrySet(CStrID ID, pass<T> Value)
	{
		if constexpr (TypeIndex<T> < sizeof...(TVarTypes))
			Set<T>(ID, Value);
		return (TypeIndex<T> < sizeof...(TVarTypes));
	}

	template<typename T, typename std::enable_if_t<!std::is_same_v<pass<T>, T>>* = nullptr>
	HVar TrySet(CStrID ID, T&& Value)
	{
		if constexpr (TypeIndex<T> < sizeof...(TVarTypes))
			Set<T>(ID, std::move(Value));
		return (TypeIndex<T> < sizeof...(TVarTypes));
	}

public:

	HVar Find(CStrID ID) const
	{
		const auto It = _VarsByID.find(ID);
		return (It == _VarsByID.cend()) ? InvalidVar : It->second;
	}

	void clear()
	{
		_VarsByID.clear();
		std::apply([](auto& ...Storage) { (..., Storage.clear()); }, _Storages);
	}

	bool empty() const { return _VarsByID.empty(); }
	auto size() const { return _VarsByID.size(); }

	template<typename T>
	pass<T> Get(HVar Handle) const
	{
		// Explicit check saves us from a spam of compiler errors with the same meaning from std::get
		static_assert(DEM::Meta::contains_type<T, TVarTypes...>(), "Requested type is not supported by this storage");
		n_assert_dbg(IsTypeValid<T>(Handle));
		return std::get<std::vector<T>>(_Storages)[Handle & ((1 << VAR_INDEX_BITS) - 1)];
	}

	template<typename T>
	void Set(HVar Handle, pass<T> Value)
	{
		// TODO: support convertible here too?
		n_assert_dbg(IsTypeValid<T>(Handle));
		if (IsTypeValid<T>(Handle))
			std::get<TypeIndex<T>>(_Storages)[Handle & ((1 << VAR_INDEX_BITS) - 1)] = Value;
	}

	template<typename T, typename std::enable_if_t<!std::is_same_v<pass<T>, T>>* = nullptr>
	void Set(HVar Handle, T&& Value)
	{
		// TODO: support convertible here too?
		n_assert_dbg(IsTypeValid<T>(Handle));
		if (IsTypeValid<T>(Handle))
			std::get<TypeIndex<T>>(_Storages)[Handle & ((1 << VAR_INDEX_BITS) - 1)] = std::move(Value);
	}

	//!!!TODO: check code generation between pass<T> and T&& for by-value types! Maybe don't need these two versions!
	//!!!also currently it can't deduce var type from arg! Need to write Set<float> etc!
	template<typename T>
	HVar Set(CStrID ID, pass<T> Value)
	{
		static_assert(TypeIndex<T> < sizeof...(TVarTypes), "Requested type is not supported by this storage nor it can be converted to a supported type");

		auto It = _VarsByID.find(ID);
		if (It == _VarsByID.cend())
		{
			auto& Storage = std::get<TypeIndex<T>>(_Storages);
			const HVar Handle = (TypeIndex<T> << VAR_INDEX_BITS) | Storage.size();
			It = _VarsByID.emplace(ID, Handle).first;
			Storage.push_back(Value);
		}
		else
		{
			Set<T>(It->second, Value);
		}

		return It->second;
	}

	template<typename T, typename std::enable_if_t<!std::is_same_v<pass<T>, T>>* = nullptr>
	HVar Set(CStrID ID, T&& Value)
	{
		static_assert(TypeIndex<T> < sizeof...(TVarTypes), "Requested type is not supported by this storage nor it can be converted to a supported type");

		auto It = _VarsByID.find(ID);
		if (It == _VarsByID.cend())
		{
			auto& Storage = std::get<TypeIndex<T>>(_Storages);
			const HVar Handle = (TypeIndex<T> << VAR_INDEX_BITS) | Storage.size();
			It = _VarsByID.emplace(ID, Handle).first;
			Storage.push_back(std::move(Value));
		}
		else
		{
			Set<T>(It->second, std::move(Value));
		}

		return It->second;
	}

	size_t Load(const Data::CParams& Params)
	{
		size_t Loaded = 0;
		for (const auto& Param : Params)
		{
			switch (Param.GetRawValue().GetTypeID())
			{
				case Data::CTypeID<bool>::TypeID: Loaded += TrySet<bool>(Param.GetName(), Param.GetValue<bool>()); break;
				case Data::CTypeID<int>::TypeID: Loaded += TrySet<int>(Param.GetName(), Param.GetValue<int>()); break;
				case Data::CTypeID<float>::TypeID: Loaded += TrySet<float>(Param.GetName(), Param.GetValue<float>()); break;
				case Data::CTypeID<CString>::TypeID:
				{
					const auto& Src = Param.GetValue<CString>();
					if constexpr (DEM::Meta::contains_type<CString, TVarTypes...>())
						Loaded += TrySet<CString>(Param.GetName(), Src);
					else
						Loaded += TrySet<std::string>(Param.GetName(), std::string(Src.CStr(), Src.GetLength()));
					break;
				}
				case Data::CTypeID<CStrID>::TypeID: Loaded += TrySet<CStrID>(Param.GetName(), Param.GetValue<CStrID>()); break;
				case Data::CTypeID<vector3>::TypeID: Loaded += TrySet<vector3>(Param.GetName(), Param.GetValue<vector3>()); break;
				case Data::CTypeID<vector4>::TypeID: Loaded += TrySet<vector4>(Param.GetName(), Param.GetValue<vector4>()); break;
				case Data::CTypeID<matrix44>::TypeID: Loaded += TrySet<matrix44>(Param.GetName(), Param.GetValue<matrix44>()); break;
				case Data::CTypeID<Data::PParams>::TypeID: Loaded += TrySet<Data::PParams>(Param.GetName(), Param.GetValue<Data::PParams>()); break;
				case Data::CTypeID<Data::PDataArray>::TypeID: Loaded += TrySet<Data::PDataArray>(Param.GetName(), Param.GetValue<Data::PDataArray>()); break;
				default: break;
			}
		}

		return Loaded;
	}

	size_t Save(Data::CParams& Params)
	{
		size_t Saved = 0;
		for (const auto& [ID, Handle] : _VarsByID)
		{
			const auto CurrTypeIndex = (Handle >> VAR_INDEX_BITS);
			Saved += DEM::Meta::compile_switch(CurrTypeIndex, std::index_sequence_for<TVarTypes...>{}, [this, &Params, ID = ID, Handle = Handle](auto i) {
				using TVarType = std::tuple_element_t<i, std::tuple<TVarTypes...>>;
				using THRDType = Data::THRDType<TVarType>;

				// Left here temporary to control usage. Can remove and allow to skip saving not supported types.
				static_assert(Data::CTypeID<THRDType>::IsDeclared);

				if constexpr (!Data::CTypeID<THRDType>::IsDeclared)
					return false;

				if constexpr (std::is_convertible_v<TVarType, THRDType>)
					Params.Set(ID, Get<TVarType>(Handle));
				else
					Params.Set(ID, THRDType(Get<TVarType>(Handle)));

				return true;
			});
		}

		return Saved;
	}
};
