#pragma once
#include <AI/Blackboard.h>

// An AI agent's parameter that can be set directly or read from a blackboard key
// CParameter   - a simple mandatory (non-optional) strictly typed parameter. Has better memory footprint for sizeof(BBKey) == sizeof(T) cases.
// CParameterEx - can be optional, can accept different types like std::variant.

// TODO: can use HVar instead of CStrID for BBKey? What if key is added or deleted from BB? Make a real handle from HVar, with gen counter? And/or subscribe BB changes?

namespace DEM::AI
{
inline const auto sidBBKey = CStrID("BBKey");
inline const auto sidDefault = CStrID("Default");
inline const auto sidValue = CStrID("Value");

template<typename T>
class CParameter
{
protected:

	using TPass = std::conditional_t<DEM::Meta::should_pass_by_value<T>, T, const T&>;

	// NB: sizeof(CParameter<T>) == sizeof(std::variant<BBKey, T>), so we chose to keep both side
	//     by side instead of switching between them. When BB is in use, Value serves as a default.
	CStrID _BBKey;
	T      _Value = {};

public:

	CParameter(TPass Value = {}) : _Value(Value) {}
	CParameter(T&& Value) noexcept : _Value(std::move(Value)) {}
	CParameter(CStrID BBKey, TPass Default = {}) : _BBKey(BBKey), _Value(Default) {}
	CParameter(CStrID BBKey, T&& Default = {}) : _BBKey(BBKey), _Value(std::move(Default)) {}

	CParameter(const Data::CData& Data)
	{
		if (auto* pBBKey = Data.As<CStrID>())
		{
			// For consistency CStrID is always interpreted as a BB key, even if it is also T
			_BBKey = *pBBKey;
		}
		else 
		{
			bool IsFullForm = false;
			if (auto* pParams = Data.As<Data::PParams>())
			{
				// Read a full non-ambiguous form { BBKey='Key' Default=10 } or { Value=10 }
				auto& Params = **pParams;
				IsFullForm = Params.TryGet(_BBKey, sidBBKey);
				if (IsFullForm)
					Params.TryGet(_Value, sidDefault);
				else
					IsFullForm = Params.TryGet(_Value, sidValue);
			}

			// Try reading a value directly from an argument
			if (!IsFullForm) ParamsFormat::Deserialize(Data, _Value);
		}
	}

	TPass Get(const CBlackboard& Blackboard) const
	{
		return _BBKey ? Blackboard.GetStorage().Get(Blackboard.GetStorage().Find(_BBKey), _Value) : _Value;
	}
};

template<typename... TVarTypes>
class CParameterEx final
{
private:

	template<typename T>
	using TPass = std::conditional_t<DEM::Meta::should_pass_by_value<T>, T, const T&>;

	template<typename T>
	static constexpr auto TypeIndex = DEM::Meta::contains_type<T, TVarTypes...>() ?
		DEM::Meta::index_of_type<T, TVarTypes...>() :
		DEM::Meta::index_of_type<DEM::Meta::best_conversion_t<T, TVarTypes...>, TVarTypes...>();

	static constexpr auto MaxSize = std::max({ sizeof(TVarTypes)... });
	static constexpr auto MaxAlign = std::max({ alignof(TVarTypes)... });
	static constexpr auto NoType = INVALID_INDEX_T<uint8_t>;

	static_assert(sizeof...(TVarTypes) <= NoType, "Too many variable types in a template parameter list");

	// NB: _Type combines with the smaller of _Value & _BBKey and takes additional memory only when their sizes are equal
	alignas(MaxAlign) std::byte _Value[MaxSize];
	uint8_t                     _Type = NoType;
	CStrID                      _BBKey;

	template<typename F>
	decltype(auto) Visit(F Visitor)
	{
		using TRet = std::common_type_t<decltype(Visitor(static_cast<TVarTypes*>(nullptr)))... > ;

		if (_Type == NoType) return TRet{};

		return DEM::Meta::compile_switch(static_cast<size_t>(_Type), std::index_sequence_for<TVarTypes...>{}, [this, &Visitor](auto i)
		{
			using TVarType = std::tuple_element_t<i, std::tuple<TVarTypes...>>;
			return Visitor(reinterpret_cast<TVarType*>(_Value));
		});
	}

	template<typename T>
	void InitValue(T&& Value)
	{
		static_assert(TypeIndex<T> < sizeof...(TVarTypes), "Requested type is not supported by this parameter nor it can be unambiguosly converted to a supported type");

		using TVarType = std::tuple_element_t<TypeIndex<T>, std::tuple<TVarTypes...>>;

		n_assert_dbg(_Type == NoType);

		new (_Value) TVarType(std::forward<T>(Value));
		_Type = TypeIndex<T>;
	}

	void DestroyValue()
	{
		Visit([](auto* pValue) { std::destroy_at(pValue); });
		_Type = NoType;
	}

public:

	CParameterEx() = default;
	//CParameterEx(const CParameterEx&) = delete;
	//CParameterEx(CParameterEx&&) noexcept = delete;

	template<typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, CParameterEx<TVarTypes...>>>>
	CParameterEx(T&& Value)
	{
		InitValue(std::forward<T>(Value));
	}

	CParameterEx(CStrID BBKey) : _BBKey(BBKey) { /* NB: type and value are not initialized */ }

	template<typename T>
	CParameterEx(CStrID BBKey, T&& Default)
		: _BBKey(BBKey)
	{
		InitValue(std::forward<T>(Default));
	}

	~CParameterEx()
	{
		DestroyValue();
	}

	template<typename T>
	bool IsValueA() const { return (TypeIndex<T> < sizeof...(TVarTypes)) && (_Type == TypeIndex<T>); }

	template<typename T>
	bool TryGet(const CBlackboard& Blackboard, T& Out) const
	{
		static_assert(TypeIndex<T> < sizeof...(TVarTypes), "Requested type is not supported by this parameter nor it can be unambiguosly converted to a supported type");

		// If there is data of matching type on the matching key in BB, return it
		if (_BBKey && Blackboard.GetStorage().TryGet(Blackboard.GetStorage().Find(_BBKey), Out)) return true;

		// Default to _Value. Can store and request types that are not supported by BB.
		if constexpr (TypeIndex<T> < sizeof...(TVarTypes))
		{
			if (_Type == TypeIndex<T>)
			{
				using TVarType = std::tuple_element_t<TypeIndex<T>, std::tuple<TVarTypes...>>;
				Out = *reinterpret_cast<const TVarType*>(_Value);
				return true;
			}
		}

		return false;
	}

	template<typename T>
	TPass<T> Get(const CBlackboard& Blackboard, TPass<T> Default = {}) const
	{
		// Types passed by reference must match exactly
		static_assert(!std::is_reference_v<TPass<T>> || DEM::Meta::contains_type<T, TVarTypes...>(), "Requested type is not supported by this parameter");

		// If there is data of matching type on the matching key in BB, return it
		if (_BBKey)
		{
			auto& BB = Blackboard.GetStorage();
			if (auto Handle = BB.Find(_BBKey))
				if (BB.IsA<T>(Handle))
					return BB.Get(Handle, Default); // NB: Default will not be used
		}

		// Default to _Value. Can store and request types that are not supported by BB.
		if constexpr (TypeIndex<T> < sizeof...(TVarTypes))
		{
			if (_Type == TypeIndex<T>)
			{
				using TVarType = std::tuple_element_t<TypeIndex<T>, std::tuple<TVarTypes...>>;
				return *reinterpret_cast<const TVarType*>(_Value);
			}
		}

		return Default;
	}

	// Prevent Default lifetime issues
	template<typename T, typename std::enable_if_t<!DEM::Meta::should_pass_by_value<T>>* = nullptr>
	const T& Get(const CBlackboard& Blackboard, T&& Default) const = delete;
};

}
