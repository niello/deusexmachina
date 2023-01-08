#include "SmartObject.h"
#include <Game/Interaction/InteractionContext.h>
#include <Game/Interaction/InteractionManager.h>
#include <Game/Interaction/ScriptedInteraction.h>
#include <Game/Interaction/ScriptedAbility.h>

namespace DEM::Game
{

CSmartObject::CSmartObject(CStrID ID, CStrID DefaultState, std::string_view ScriptSource,
	std::vector<CSmartObjectStateInfo>&& States, std::vector<CZone>&& Zones,
	std::map<CStrID, CFixedArray<CStrID>>&& InteractionOverrides)
	: _ID(ID)
	, _DefaultState(DefaultState)
	, _ScriptSource(ScriptSource)
	, _States(std::move(States))
	, _InteractionZones(std::move(Zones))
	, _InteractionOverrides(std::move(InteractionOverrides))
{
	// Sort states and transitions by ID
	std::sort(_States.begin(), _States.end(), [](const auto& a, const auto& b) { return a.ID < b.ID; });
	for (auto& State : _States)
		std::sort(State.Transitions.begin(), State.Transitions.end(), [](const auto& a, const auto& b) { return a.TargetStateID < b.TargetStateID; });
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

const CFixedArray<CStrID>* CSmartObject::GetInteractionOverrides(CStrID ID) const
{
	auto It = _InteractionOverrides.find(ID);
	return (It == _InteractionOverrides.cend() || It->second.empty()) ? nullptr : &It->second;
}
//---------------------------------------------------------------------

// CSmartObject is shared between sessions, and here we initlialize per-session data
bool CSmartObject::InitInSession(CGameSession& Session) const
{
	if (_ScriptSource.empty() || !_ID) return true;

	auto& Lua = Session.GetScriptState();

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

	if (auto pInteractionMgr = Session.FindFeature<CInteractionManager>())
	{
		auto Interactions = ScriptObject.get<sol::table>("Interactions");
		for (const auto& Interaction : Interactions)
		{
			if (Interaction.second.get_type() != sol::type::table) continue;

			auto IactTable = Interaction.second.as<sol::table>();

			PInteraction Iact;
			if (IactTable.get<sol::function>("Execute"))
				Iact.reset(n_new(CScriptedInteraction(IactTable)));
			else
				Iact.reset(n_new(CScriptedAbility(Lua, IactTable)));

			if (Iact)
			{
				CStrID IactID(Interaction.first.as<const char*>());
				pInteractionMgr->RegisterInteraction(IactID, std::move(Iact), _ID);
			}
		}
	}

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
