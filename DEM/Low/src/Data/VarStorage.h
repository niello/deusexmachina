#pragma once
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Data/TypeTraits.h>
#include <map>
#include <variant>

// A heterogeneous strongly typed named variable storage, like map<ID, std::variant> but more optimal in RAM.
// Features:
// - Saving and loading with Data::CParams. Can be easily extended or changed to XML, JSON or any other markup.
// - Auto-conversion of an absent type to the best matching type when possible
// - Visitation of contents similar to std::visit
// Limitations:
// - Variable, once added, can be cleared but can't be deleted. Only full storage cleanup is available.
// - No more than 15 different types and 268M variables per type, see HVar bits usage
// - Auto-conversion is disabled when ambiguous, cast your values to the desired type manually in this case

// Lightweight opaque handle for fast access to variables in CVarStorage
struct HVar
{
	static constexpr size_t TYPE_INDEX_BITS = 4;
	static constexpr size_t VAR_INDEX_BITS = 28;

	uint32_t TypeIdx : TYPE_INDEX_BITS;
	uint32_t VarIdx : VAR_INDEX_BITS;

	constexpr HVar() : TypeIdx((1 << TYPE_INDEX_BITS) - 1), VarIdx((1 << VAR_INDEX_BITS) - 1) {}
	constexpr HVar(uint32_t TypeIdx_, uint32_t VarIdx_) : TypeIdx(TypeIdx_), VarIdx(VarIdx_) {}

	constexpr operator bool() { return TypeIdx != HVar{}.TypeIdx && VarIdx != HVar{}.VarIdx; } // NB: any field invalid -> HVar invalid
};

template<typename... TVarTypes>
class CVarStorage
{
protected:

	static_assert(sizeof...(TVarTypes) < (1 << HVar::TYPE_INDEX_BITS), "Too many types to be indexed in HVar"); // NB: one value is reserved for invalid index

	template<typename T>
	using pass = std::conditional_t<(sizeof(T) > sizeof(size_t)), const T&, T>;

	template<typename T>
	static constexpr auto TypeIndex = DEM::Meta::contains_type<T, TVarTypes...>() ?
		DEM::Meta::index_of_type<T, TVarTypes...>() :
		DEM::Meta::index_of_type<DEM::Meta::best_conversion_t<T, TVarTypes...>, TVarTypes...>();

	std::tuple<std::vector<TVarTypes>...> _Storages;
	std::map<CStrID, HVar>                _VarsByID;

	template<typename T>
	bool TrySet(CStrID ID, pass<T> Value)
	{
		if constexpr (TypeIndex<T> < sizeof...(TVarTypes))
			Set<T>(ID, Value);
		return (TypeIndex<T> < sizeof...(TVarTypes));
	}

	template<typename T, typename std::enable_if_t<!std::is_same_v<pass<T>, T>>* = nullptr>
	bool TrySet(CStrID ID, T&& Value)
	{
		if constexpr (TypeIndex<T> < sizeof...(TVarTypes))
			Set<T>(ID, std::move(Value));
		return (TypeIndex<T> < sizeof...(TVarTypes));
	}

public:

	HVar Find(CStrID ID) const
	{
		const auto It = _VarsByID.find(ID);
		return (It == _VarsByID.cend()) ? HVar{} : It->second;
	}

	// NB: this invalidated all HVar handles issued by this storage
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
		n_assert_dbg(Handle.TypeIdx == TypeIndex<T>);
		return std::get<std::vector<T>>(_Storages)[Handle.VarIdx];
	}

	auto Get(HVar Handle) const
	{
		std::variant<std::monostate, TVarTypes...> Result;
		DEM::Meta::compile_switch(Handle.TypeIdx, std::index_sequence_for<TVarTypes...>{}, [this, &Result, VarIdx = Handle.VarIdx](auto i)
		{
			Result = std::get<i>(_Storages)[VarIdx];
		});
		return Result;
	}

	template<typename T>
	void Set(HVar Handle, pass<T> Value)
	{
		static_assert(TypeIndex<T> < sizeof...(TVarTypes), "Requested type is not supported by this storage nor it can be converted to a supported type");

		n_assert_dbg(Handle.TypeIdx == TypeIndex<T>);
		if (Handle.TypeIdx == TypeIndex<T>)
			std::get<TypeIndex<T>>(_Storages)[Handle.VarIdx] = Value;
	}

	template<typename T, typename std::enable_if_t<!std::is_same_v<pass<T>, T>>* = nullptr>
	void Set(HVar Handle, T&& Value)
	{
		static_assert(TypeIndex<T> < sizeof...(TVarTypes), "Requested type is not supported by this storage nor it can be converted to a supported type");

		n_assert_dbg(Handle.TypeIdx == TypeIndex<T>);
		if (Handle.TypeIdx == TypeIndex<T>)
			std::get<TypeIndex<T>>(_Storages)[Handle.VarIdx] = std::move(Value);
	}

	template<typename T>
	HVar Set(CStrID ID, pass<T> Value)
	{
		static_assert(TypeIndex<T> < sizeof...(TVarTypes), "Requested type is not supported by this storage nor it can be converted to a supported type");

		auto It = _VarsByID.find(ID);
		if (It == _VarsByID.cend())
		{
			auto& Storage = std::get<TypeIndex<T>>(_Storages);
			It = _VarsByID.emplace(ID, HVar{ static_cast<uint32_t>(TypeIndex<T>), static_cast<uint32_t>(Storage.size()) }).first;
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
			It = _VarsByID.emplace(ID, HVar{ static_cast<uint32_t>(TypeIndex<T>), static_cast<uint32_t>(Storage.size()) }).first;
			Storage.push_back(std::move(Value));
		}
		else
		{
			Set<T>(It->second, std::move(Value));
		}

		return It->second;
	}

	template<typename F>
	void Visit(F Callback)
	{
		for (const auto& [ID, Handle] : _VarsByID)
		{
			DEM::Meta::compile_switch(Handle.TypeIdx, std::index_sequence_for<TVarTypes...>{}, [this, &Callback, ID, VarIdx = Handle.VarIdx](auto i)
			{
				Callback(ID, std::get<i>(_Storages)[VarIdx]);
			});
		}
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
			Saved += DEM::Meta::compile_switch(Handle.TypeIdx, std::index_sequence_for<TVarTypes...>{}, [this, &Params, ID = ID, Handle = Handle](auto i)
			{
				using TVarType = std::tuple_element_t<i, std::tuple<TVarTypes...>>;
				using THRDType = Data::THRDType<TVarType>;

				// Left here temporary to control usage. Can remove and allow to skip saving not supported types.
				static_assert(Data::CTypeID<THRDType>::IsDeclared);

				if constexpr (!Data::CTypeID<THRDType>::IsDeclared)
					return false;

				if constexpr (std::is_convertible_v<TVarType, THRDType>)
					Params.Set(ID, static_cast<THRDType>(Get<TVarType>(Handle)));
				else
					Params.Set(ID, THRDType(Get<TVarType>(Handle)));

				return true;
			});
		}

		return Saved;
	}
};
