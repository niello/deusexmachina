#include "SmartObject.h"

namespace DEM::Game
{
RTTI_CLASS_IMPL(DEM::Game::CSmartObject, Resources::CResourceObject);

CSmartObject::CSmartObject(CStrID DefaultState, std::string_view ScriptSource)
	: _ScriptSource(ScriptSource)
	, _DefaultState(DefaultState)
{
}
//---------------------------------------------------------------------

bool CSmartObject::AddState(CStrID ID, CTimelineTask&& TimelineTask/*, state logic object ptr (optional)*/)
{
	// Check if state with the same ID exists
	auto It = std::lower_bound(_States.begin(), _States.end(), ID, [](const auto& Elm, CStrID Value) { return Elm.ID < Value; });
	if (It != _States.end() && (*It).ID == ID) return false;

	// Insert sorted by ID
	_States.insert(It, CStateRecord{ ID, std::move(TimelineTask) });

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
	auto It2 = std::lower_bound(It->Transitions.begin(), It->Transitions.end(), ToID, [](const auto& Elm, CStrID Value) { return Elm.first < Value; });
	if (It2 != It->Transitions.end() && (*It2).first == ToID) return false;

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

bool CSmartObject::InitScript(sol::state& Lua)
{
	if (_ScriptSource.empty()) return true;

	//???cache env as field? or functions are enough? or don't cache functions, only env?
	sol::environment ScriptObject(Lua, sol::create);
	auto Result = Lua.script(_ScriptSource, ScriptObject /*, chunk name from SO ID*/);
	if (!Result.valid())
	{
		sol::error Error = Result;
		::Sys::Error(Error.what());
		return false;
	}

	// save the table as named object in Lua (namespace it like Smart.<MyID>)
	// cache state functions
	// cache interaction condition functions

	NOT_IMPLEMENTED;
	return false;
}
//---------------------------------------------------------------------

}