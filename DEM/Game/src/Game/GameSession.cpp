#include "GameSession.h"

namespace DEM::Game
{

CGameSession::CGameSession()
{
	_ScriptState.open_libraries(sol::lib::base);

	_ScriptState["print"] = [](lua_State* L)
	{
		::Sys::Log("Lua > ");
		const int n = lua_gettop(L); // number of arguments
		for (int i = 1; i <= n; ++i)
		{
			if (i > 1) ::Sys::Log("\t");
			if (auto s = lua_tostring(L, i))
				::Sys::Log(s);
			else
				::Sys::Log((std::string("Non-printable object of type ") + lua_typename(L, (lua_type(L, i)))).c_str());
		}
		::Sys::Log("\n");
		return 0;
	};

	//???does index work with polymorphic types?
	//!!!it doesn't, so session API is not very useful, at least FindFeature, becaus it requires us to
	//get RTTI and write switch-case for every derived type, which is stupid.
	auto SessionClassMeta = _ScriptState.new_usertype<CGameSession>("CGameSession"
		, sol::meta_function::index, [](CGameSession& Self, const char* pKey) { return Self._ScriptFields[pKey]; }
		, "FindFeature", sol::overload(
			static_cast<::Core::CRTTIBaseClass* (CGameSession::*)(const char*) const>(&CGameSession::FindFeature)
			, static_cast<::Core::CRTTIBaseClass* (CGameSession::*)(CStrID) const>(&CGameSession::FindFeature))
		);

	_ScriptState.new_usertype<CStrID>("CStrID"
		, sol::constructors<sol::types<>, sol::types<const char*>>()
		, sol::meta_function::to_string, &CStrID::CStr
		, sol::meta_function::length, [](CStrID ID) { return ID.CStr() ? strlen(ID.CStr()) : 0; }
		, sol::meta_function::index, [](CStrID ID, size_t i) { return ID.CStr() ? ID.CStr()[i - 1] : '\0'; }
		, sol::meta_function::concatenation, sol::overload(
			[](const char* a, CStrID b) { return std::string(a) + b.CStr(); }
			, [](CStrID a, const char* b) { return std::string(a.CStr()) + b; }
			, [](CStrID a, CStrID b) { return std::string(a.CStr()) + b.CStr(); })
		, sol::meta_function::equal_to, [](CStrID a, CStrID b) { return a == b; }
		, "Empty", []() { return CStrID::Empty; }
		);
	//sol::table StrIDClassTable = _ScriptState["CStrID"];
	//StrIDClassTable.set("Empty", CStrID::Empty);

	_ScriptFields = _ScriptState.create_table();
	_ScriptState["Session"] = this;
}
//---------------------------------------------------------------------

CGameSession::~CGameSession()
{
	// Before destroying Lua, according to init order
	_FeaturesByName.clear(); //???unregister features from Lua one by one, don't leave dangling pointer even for a moment?
	_Features.clear();

	_ScriptState["Session"] = sol::nil;
}
//---------------------------------------------------------------------

}
