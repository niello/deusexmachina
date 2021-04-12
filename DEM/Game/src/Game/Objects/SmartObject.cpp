#include "SmartObject.h"
#include <Game/Interaction/InteractionContext.h>
#include <sol/sol.hpp>

namespace DEM::Game
{

CSmartObject::CSmartObject(CStrID ID, CStrID DefaultState, bool Static, std::string_view ScriptSource,
	std::vector<CSmartObjectStateInfo>&& States, std::vector<CInteractionZone>&& InteractionZones)
	: _ID(ID)
	, _DefaultState(DefaultState)
	, _ScriptSource(ScriptSource)
	, _States(std::move(States))
	, _InteractionZones(std::move(InteractionZones))
	, _Static(Static)
{
	n_assert_dbg(_InteractionZones.size() <= MAX_ZONES);

	// Sort states and transitions by ID
	std::sort(_States.begin(), _States.end(), [](const auto& a, const auto& b) { return a.ID < b.ID; });
	for (auto& State : _States)
		std::sort(State.Transitions.begin(), State.Transitions.end(), [](const auto& a, const auto& b) { return a.TargetStateID < b.TargetStateID; });

	// Collect a set of all interactions available for this object
	std::set<CStrID> Interactions;
	for (const auto& Zone : _InteractionZones)
		for (const auto& Interaction : Zone.Interactions)
			Interactions.insert(Interaction.ID);

	// Save interaction set into a memory efficient array, already sorted and deduplicated
	_Interactions.SetSize(Interactions.size());
	std::copy(Interactions.begin(), Interactions.end(), _Interactions.begin());
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

bool CSmartObject::HasInteraction(CStrID ID) const
{
	auto It = std::lower_bound(_Interactions.begin(), _Interactions.end(), ID, [](const auto& Elm, CStrID Value) { return Elm < Value; });
	return It != _Interactions.end() && *It == ID;
}
//---------------------------------------------------------------------

CStrID CSmartObject::GetInteractionOverride(CStrID ID, const CInteractionContext& Context) const
{
	//!!!DBG TMP!
	return Context.Tool == CStrID("DefaultAction") ? CStrID("Open") : CStrID::Empty;

	auto It = _InteractionOverrides.find(ID);
	if (It == _InteractionOverrides.cend()) return CStrID::Empty;

	for (const CStrID ID : It->second)
	{
		// if iact with this ID is available in a Context
		//(???test here or in interaction manager? may override into a global action, not SO!)
		//!!!but override based on a list must select first available interaction! DefaultAction -> Open, Close

		/* OLD:
			auto Condition = pSmartAsset->GetScriptFunction(Context.Session->GetScriptState(), "Can" + std::string(ID.CStr()));
			auto pInteraction = ValidateInteraction(ID, Condition, Context);
			if (pInteraction &&
				pInteraction->GetMaxTargetCount() > 0 &&
				pInteraction->GetTargetFilter(0)->IsTargetValid(Context))
			{
				Context.Interaction = ID;
				Context.Condition = Condition;
				return true;
			}
		*/
	}

	return CStrID::Empty;
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
	//???FIXME: can write shorter?
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
