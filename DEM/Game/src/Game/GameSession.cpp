#include "GameSession.h"
#include <Scripting/ScriptAsset.h>
#include <Resources/Resource.h>

namespace DEM::Game
{

CGameSession::CGameSession(Resources::CResourceManager& ResMgr)
	: _ResMgr(ResMgr)
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

// TODO: can move it to script asset loading if script state will be per application and not per session
sol::table CGameSession::GetScript(CStrID ID, bool ForceReload)
{
	if (!ID) return sol::nil;

	if (!ForceReload)
	{
		const auto It = _LoadedScripts.find(ID);
		if (It != _LoadedScripts.cend()) return It->second;
	}

	auto ScriptResource = _ResMgr.RegisterResource<DEM::Scripting::CScriptAsset>(ID.CStr());
	if (!ScriptResource) return sol::nil;

	auto pScriptAsset = ScriptResource->ValidateObject<DEM::Scripting::CScriptAsset>();
	if (!pScriptAsset) return sol::nil;

	sol::environment ScriptObject(_ScriptState, sol::create, _ScriptState.globals());
	auto Result = _ScriptState.script(pScriptAsset->GetSourceBuffer(), ScriptObject, std::string(ID.CStr()), sol::load_mode::any);
	if (!Result.valid())
	{
		sol::error Error = Result;
		::Sys::Error(Error.what());
		return sol::nil;
	}

	auto ScriptAssetRegistry = _ScriptState["ScriptAssets"].get_or_create<sol::table>();
	ScriptAssetRegistry[ID.CStr()] = ScriptObject;

	_LoadedScripts.insert_or_assign(ID, ScriptObject);

	return ScriptObject;
}
//---------------------------------------------------------------------

}
