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
				::Sys::Log("<unknown>");
		}
		::Sys::Log("\n");
		return 0;
	};

	//???does index work with polymorphic types?
	auto SessionClassMeta = _ScriptState.new_usertype<CGameSession>("CGameSession"
		//, sol::meta_function::index, [](CGameSession& Self, const char* pKey) { return Self.FindFeature(pKey); }
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
		);

	//???or separate table for features? Or in globals?Or Session is a table without usertype methods, only functions?
	//Session instance may still be needed!
	auto SessionInstanceMeta = _ScriptState.create_table();
	SessionInstanceMeta[sol::meta_function::index] = this;
	SessionInstanceMeta[sol::metatable_key] = SessionClassMeta;
	auto SessionTable = _ScriptState.create_table();
	SessionTable[sol::metatable_key] = SessionInstanceMeta;
	_ScriptState["Session"] = SessionTable;

	//_ScriptState["Session"] = this;
}
//---------------------------------------------------------------------

CGameSession::~CGameSession()
{
	// Before destroying Lua, according to init order
	_FeaturesByName.clear();
	_Features.clear();

	_ScriptState["Session"] = sol::nil;
}
//---------------------------------------------------------------------

}
