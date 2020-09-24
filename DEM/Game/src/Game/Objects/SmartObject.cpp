#include "SmartObject.h"
#include <sol/sol.hpp>

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
	if (!ID) return false;

	// Check if state with the same ID exists
	auto It = std::lower_bound(_States.begin(), _States.end(), ID, [](const auto& Elm, CStrID Value) { return Elm.ID < Value; });
	if (It != _States.end() && (*It).ID == ID) return false;

	// Insert sorted by ID
	_States.insert(It, { ID, std::move(TimelineTask) });

	return true;
}
//---------------------------------------------------------------------

bool CSmartObject::AddTransition(CStrID FromID, CStrID ToID, CTimelineTask&& TimelineTask, ETransitionInterruptionMode InterruptionMode)
{
	if (!FromID || !ToID) return false;

	// Find source state, it must exist
	auto It = std::lower_bound(_States.begin(), _States.end(), FromID, [](const auto& Elm, CStrID Value) { return Elm.ID < Value; });
	if (It == _States.end() || (*It).ID != FromID) return false;

	// FIXME: for now don't check dest state existence due to loading order in CSmartObjectLoader

	// Check if this transition already exists
	auto It2 = std::lower_bound(It->Transitions.begin(), It->Transitions.end(), ToID, [](const auto& Elm, CStrID Value) { return Elm.TargetStateID < Value; });
	if (It2 != It->Transitions.end() && (*It2).TargetStateID == ToID) return false;

	// Insert sorted by ID
	It->Transitions.insert(It2, { ToID, std::move(TimelineTask), InterruptionMode });

	return true;
}
//---------------------------------------------------------------------

bool CSmartObject::AddInteraction(CStrID ID)
{
	if (!ID) return false;

	// Check if this interaction exists
	auto It = std::lower_bound(_Interactions.begin(), _Interactions.end(), ID, [](const auto& Elm, CStrID Value) { return Elm < Value; });
	if (It != _Interactions.end() && *It == ID) return false;

	// Insert sorted by ID
	_Interactions.insert(It, ID);

	return true;
}
//---------------------------------------------------------------------

const CSmartObjectStateInfo* CSmartObject::FindState(CStrID ID) const
{
	// Find source state, it must exist
	auto It = std::lower_bound(_States.begin(), _States.end(), ID, [](const auto& Elm, CStrID Value) { return Elm.ID < Value; });
	return (It == _States.end() || (*It).ID != ID) ? nullptr : &(*It);
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
	auto Result = Lua.script(_ScriptSource, ScriptObject); //!!!chunk name conflicts with another signature! , std::string(_ID.CStr()));
	if (!Result.valid())
	{
		sol::error Error = Result;
		::Sys::Error(Error.what());
		return false;
	}

	//???use sol::this_environment for calls as a Self ref?

	//???need to add new_usertype for CSmartObject to ScriptObject or to global table?

	// Write new script object into a smart object registry
	auto SmartObjectsRegistry = Lua["SmartObjects"].get_or_create<sol::table>();
	SmartObjectsRegistry[_ID.CStr()] = ScriptObject;

	return true;
}
//---------------------------------------------------------------------

// NB: can't cache Lua objects here because Lua state is per-session and resource is per-application
sol::function CSmartObject::GetScriptFunction(sol::state& Lua, std::string_view Name) const
{
	auto ObjProxy = Lua["SmartObjects"][_ID.CStr()];
	if (ObjProxy.get_type() == sol::type::table)
	{
		sol::table ScriptObject = ObjProxy;
		auto FnProxy = ScriptObject[Name];
		if (FnProxy.get_type() == sol::type::function) return FnProxy;
	}

	return sol::function();
}
//---------------------------------------------------------------------

}
