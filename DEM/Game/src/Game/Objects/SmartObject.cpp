#include "SmartObject.h"

namespace DEM::Game
{
RTTI_CLASS_IMPL(DEM::Game::CSmartObject, Resources::CResourceObject);

CSmartObject::CSmartObject(CStrID ID, CStrID DefaultState, std::string_view ScriptSource)
	: _ID(ID)
	, _DefaultState(DefaultState)
	, _ScriptSource(ScriptSource)
{
}
//---------------------------------------------------------------------

bool CSmartObject::AddState(CStrID ID, CTimelineTask&& TimelineTask/*, state logic object ptr (optional)*/)
{
	// Check if state with the same ID exists
	auto It = std::lower_bound(_States.begin(), _States.end(), ID, [](const auto& Elm, CStrID Value) { return Elm.ID < Value; });
	if (It != _States.end() && (*It).ID == ID) return false;

	// Insert sorted by ID
	_States.insert(It, { ID, std::move(TimelineTask) });

	return true;
}
//---------------------------------------------------------------------

bool CSmartObject::AddTransition(CStrID FromID, CStrID ToID, CTimelineTask&& TimelineTask)
{
	// Find source state, it must exist
	auto It = std::lower_bound(_States.begin(), _States.end(), FromID, [](const auto& Elm, CStrID Value) { return Elm.ID < Value; });
	if (It == _States.end() || (*It).ID != FromID) return false;

	// FIXME: for now don't check dest state existence due to loading order in CSmartObjectLoader

	// Check if this transition already exists
	auto It2 = std::lower_bound(It->Transitions.begin(), It->Transitions.end(), ToID, [](const auto& Elm, CStrID Value) { return Elm.TargetStateID < Value; });
	if (It2 != It->Transitions.end() && (*It2).TargetStateID == ToID) return false;

	// Insert sorted by ID
	It->Transitions.insert(It2, { ToID, std::move(TimelineTask) });

	return true;
}
//---------------------------------------------------------------------

bool CSmartObject::AddInteraction(CStrID ID)
{
	// Check if this interaction exists
	auto It = std::lower_bound(_Interactions.begin(), _Interactions.end(), ID, [](const auto& Elm, CStrID Value) { return Elm.first < Value; });
	if (It != _Interactions.end() && (*It).first == ID) return false;

	// Insert sorted by ID
	_Interactions.insert(It, { ID, sol::function() });

	return true;
}
//---------------------------------------------------------------------

const CSmartObjectTransitionInfo* CSmartObject::FindTransition(CStrID FromID, CStrID ToID) const
{
	// Find source state, it must exist
	auto It = std::lower_bound(_States.begin(), _States.end(), FromID, [](const auto& Elm, CStrID Value) { return Elm.ID < Value; });
	if (It == _States.end() || (*It).ID != FromID) return nullptr;

	// Check if this transition already exists
	auto It2 = std::lower_bound(It->Transitions.begin(), It->Transitions.end(), ToID, [](const auto& Elm, CStrID Value) { return Elm.TargetStateID < Value; });
	if (It2 == It->Transitions.end() || (*It2).TargetStateID != ToID) return nullptr;

	return &(*It2);
}
//---------------------------------------------------------------------

bool CSmartObject::InitScript(sol::state& Lua)
{
	if (_ScriptSource.empty() || !_ID) return true;

	//???cache ScriptObject as field? or functions are enough? or don't cache functions, only env?
	sol::environment ScriptObject(Lua, sol::create, Lua.globals());
	auto Result = Lua.script(_ScriptSource, ScriptObject /*, chunk name from SO ID*/);
	if (!Result.valid())
	{
		sol::error Error = Result;
		::Sys::Error(Error.what());
		return false;
	}

	auto Proxy = ScriptObject["OnStateEnter"];
	if (Proxy.get_type() == sol::type::function)
		_OnStateEnter = Proxy;

	Proxy = ScriptObject["OnStateStartEntering"];
	if (Proxy.get_type() == sol::type::function)
		_OnStateStartEntering = Proxy;

	Proxy = ScriptObject["OnStateExit"];
	if (Proxy.get_type() == sol::type::function)
		_OnStateExit = Proxy;

	Proxy = ScriptObject["OnStateStartExiting"];
	if (Proxy.get_type() == sol::type::function)
		_OnStateStartExiting = Proxy;

	Proxy = ScriptObject["OnStateUpdate"];
	if (Proxy.get_type() == sol::type::function)
		_OnStateUpdate = Proxy;

	for (auto& [ID, Condition] : _Interactions)
	{
		Proxy = ScriptObject["Can" + std::string(ID.CStr())];
		if (Proxy.get_type() == sol::type::function)
			Condition = Proxy;
	}

	//???use sol::this_environment for calls as a Self ref?

	//???new_usertype for CSmartObject in ScriptObject or in global table?

	// Write new script object into a smart object registry
	auto SmartObjectsRegistry = Lua["SmartObjects"].get_or_create<sol::table>();
	SmartObjectsRegistry[_ID.CStr()] = ScriptObject;

	return true;
}
//---------------------------------------------------------------------

}
