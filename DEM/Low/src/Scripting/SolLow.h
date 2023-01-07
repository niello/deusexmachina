#pragma once
#include <sol/sol.hpp>
#include <Data/StringID.h>

// Wrapper for Sol header with template overrides required for DEM Low layer

namespace sol
{
// FIXME: sol 3.3.0 - we may want explicit T& to be passed by ref, but sol checks trait for T& when passing T
template <> struct is_value_semantic_for_function<CStrID> : std::true_type {};
template <> struct is_value_semantic_for_function<const CStrID> : std::true_type {};
template <> struct is_value_semantic_for_function<CStrID&> : std::true_type {};
template <> struct is_value_semantic_for_function<const CStrID&> : std::true_type {};
}

namespace DEM::Scripting
{

void RegisterBasicTypes(sol::state& State);

template<typename F>
void ObjectToString(const sol::object& Object, sol::state_view& State, F Callback)
{
	const auto ObjType = Object.get_type();
	if (ObjType == sol::type::string)
	{
		Callback(Object.as<const char*>());
	}
	else
	{
		auto Result = State["tostring"](Object);
		if (Result.valid() && Result.get_type() == sol::type::string)
		{
			Callback(Result.get<const char*>());
		}
		else
		{
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

			Callback(("Non-printable object of type " + TypeName).c_str());
		}
	}
}

}
