#include "GameSession.h"

namespace DEM::Game
{

CGameSession::CGameSession()
{
	_ScriptState.open_libraries(sol::lib::base);

	//???does index work with polymorphic types?
	//!!!it doesn't, so session API is not very useful, at least FindFeature, becaus it requires us to
	//get RTTI and write switch-case for every derived type, which is stupid.
	auto SessionClassMeta = _ScriptState.new_usertype<CGameSession>("CGameSession"
		, sol::meta_function::index, [](CGameSession& Self, sol::stack_object Key) { return sol::object(Self._ScriptFields[Key]); }
		, "FindFeature", sol::overload(
			static_cast<::Core::CRTTIBaseClass* (CGameSession::*)(const char*) const>(&CGameSession::FindFeature)
			, static_cast<::Core::CRTTIBaseClass* (CGameSession::*)(CStrID) const>(&CGameSession::FindFeature))
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
