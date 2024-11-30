#pragma once
#include <sol/sol.hpp>
#include <Data/Ptr.h>
#include <Data/StringUtils.h>
#include <Data/Metadata.h>

// Wrapper for Sol header with template overrides required for DEM Low layer

using namespace std::string_view_literals;

namespace sol
{
// FIXME: sol 3.3.0 - we may want explicit T& to be passed by ref, but sol checks trait for T& when passing T
template <> struct is_value_semantic_for_function<CStrID> : std::true_type {};
template <> struct is_value_semantic_for_function<const CStrID> : std::true_type {};
template <> struct is_value_semantic_for_function<CStrID&> : std::true_type {};
template <> struct is_value_semantic_for_function<const CStrID&> : std::true_type {};

template <typename T>
struct unique_usertype_traits<Ptr<T>>
{
	typedef T type;
	typedef Ptr<T> actual_type;
	static constexpr bool value = true;

	static bool  is_null(const actual_type& ptr) { return !ptr; }
	static type* get(const actual_type& ptr) { return ptr.Get(); }
};

}

namespace DEM::Scripting
{

void RegisterBasicTypes(sol::state& State);

// FIXME: sol::create_if_nil / sol::update_if_empty do not work in a loop
inline sol::table EnsureTable(sol::state_view& State, sol::table Parent, std::string_view Name)
{
	auto Table = Parent[Name];
	if (!Table.valid())
	{
		Table = State.create_table();
		Parent.set(Name, Table);
	}
	return Table;
}
//---------------------------------------------------------------------

template<typename T>
void RegisterSignalType(sol::state& State)
{
	using TSignal = Events::CSignal<T>;
	State.new_usertype<TSignal>(sol::detail::demangle<TSignal>()
		, "Subscribe", &TSignal::Subscribe<sol::function>
		, "SubscribeAndForget", &TSignal::SubscribeAndForget<sol::function>
		, "UnsubscribeAll", &TSignal::UnsubscribeAll
		, "Empty", &TSignal::Empty
	);
}
//---------------------------------------------------------------------

template<typename T>
void RegisterStringOperations(sol::usertype<T>& UserType)
{
	// TODO: need a trait for the matching overload type or address to bind ToString directly!
	// Now casting fails because some overloads accept T and others accept const T&.
	UserType.set(sol::meta_function::to_string, [](const T& Value) { return StringUtils::ToString(Value); });

	UserType.set(sol::meta_function::concatenation, sol::overload(
		[](std::string_view a, const T& b)
		{
			// TODO: ToString writing to the Out param would help here!
			std::string bStr = StringUtils::ToString(b);
			std::string Result;
			Result.reserve(a.size() + bStr.size());
			return Result.append(a).append(bStr);
		},
		[](const T& a, std::string_view b) { return StringUtils::ToString(a).append(b); }));
}
//---------------------------------------------------------------------

template<typename T>
sol::table Namespace(sol::state_view& State)
{
	sol::table Table = State.globals();
	if constexpr (Meta::CMetadata<T>::IsRegistered)
	{
		Meta::CMetadata<T>::ForEachNamespace([&Table, &State](size_t /*Index*/, std::string_view NamespaceName)
		{
			Table = EnsureTable(State, Table, NamespaceName);
		});
	}
	return Table;
}
//---------------------------------------------------------------------

template<typename T>
void RegisterMetadataFields(sol::usertype<T>& UserType)
{
	if constexpr (Meta::CMetadata<T>::IsRegistered)
	{
		Meta::CMetadata<T>::ForEachMember([&UserType](const auto& Member)
		{
			UserType.set(Member.GetName(), sol::property(Member.GetGetter(), Member.GetSetter()));
		});
	}
}
//---------------------------------------------------------------------

template<typename T, typename... TArgs>
sol::usertype<T> RegisterTypeWithMetadata(sol::state_view& State, TArgs&&... Args)
{
	static_assert(Meta::CMetadata<T>::IsRegistered);

	auto UserType = Namespace<T>(State).new_usertype<T>(Meta::CMetadata<T>::GetUnqualifiedClassName(), std::forward<TArgs>(Args)...);
	RegisterMetadataFields(UserType);
	return UserType;
}
//---------------------------------------------------------------------

template<typename F>
auto ObjectToString(const sol::object& Object, sol::state_view& State, F Callback)
{
	const auto ObjType = Object.get_type();
	switch (ObjType)
	{
		case sol::type::none:    return Callback(""sv);
		case sol::type::nil:     return Callback("nil"sv);
		case sol::type::string:  return Callback(Object.as<std::string_view>());
		case sol::type::boolean: return Callback(StringUtils::ToString(Object.as<bool>()));
		case sol::type::number:
		{
			if (Object.is<size_t>()) return Callback(StringUtils::ToString(Object.as<size_t>()));
			if (Object.is<intptr_t>()) return Callback(StringUtils::ToString(Object.as<intptr_t>()));
			return Callback(StringUtils::ToString(Object.as<double>()));
		}
		default:
		{
			// Try converting an object via Lua tostring. For DEM classes it is frequently bound to StringUtils::ToString(T).
			auto Result = State["tostring"](Object);
			if (Result.valid() && Result.get_type() == sol::type::string)
				return Callback(Result.get<std::string_view>());

			// Print a name of a non-printable type for easier debugging
			std::string TypeName;
			if (ObjType == sol::type::table || ObjType == sol::type::userdata)
			{
				sol::optional<std::string> TypeNameObj = Object.as<sol::table>()["__type"]["name"];
				if (TypeNameObj)
					TypeName = TypeNameObj.value();
				else
					TypeName = "unknown";
			}
			else
			{
				TypeName = sol::type_name(State, ObjType);
			}

			return Callback("Non-printable object of type " + TypeName);
		}
	}
}
//---------------------------------------------------------------------

template<typename TRet = bool, typename... TArgs>
TRet LuaCall(const sol::function& Fn, TArgs&&... Args)
{
	if (!Fn) return TRet{};

	auto Result = Fn(std::forward<TArgs>(Args)...);
	if (!Result.valid())
	{
		::Sys::Error(Result.get<sol::error>().what());
		return TRet{};
	}

	if constexpr (std::is_same_v<TRet, bool>)
	{
		const auto Type = Result.get_type();
		return Type != sol::type::nil && Type != sol::type::none && Result;
	}
	else if constexpr (!std::is_same_v<TRet, void>)
	{
		return Result.get<TRet>();
	}
}
//---------------------------------------------------------------------

}
