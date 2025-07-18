#include "ScriptedAbility.h"
#include <Game/ECS/GameWorld.h>
#include <Game/Interaction/Zone.h>

namespace DEM::Game
{

CScriptedAbility::CScriptedAbility(sol::state_view Lua, const sol::table& Table)
	: _Lua(Lua)
{
	_FnIsAvailable = Table.get<sol::function>("IsAvailable");
	_FnIsTargetValid = Table.get<sol::function>("IsTargetValid");
	_FnPrepare = Table.get<sol::function>("Prepare");

	_FnGetZones = Table.get<sol::function>("GetZones");
	_FnGetFacingParams = Table.get<sol::function>("GetFacingParams");
	_FnOnStart = Table.get<sol::function>("OnStart");
	_FnOnUpdate = Table.get<sol::function>("OnUpdate");
	_FnOnEnd = Table.get<sol::function>("OnEnd");

	_CursorImage = Table.get_or<std::string>("CursorImage", {}); //???to method? pass target index?
	_MandatoryTargets = Table.get_or<U8>("MandatoryTargets", 1);
	_OptionalTargets = Table.get_or<U8>("OptionalTargets", 0);
}
//---------------------------------------------------------------------

PAbilityInstance CScriptedAbility::CreateInstance(const CInteractionContext& Context) const
{
	Ptr<CScriptedAbilityInstance> Instance(n_new(CScriptedAbilityInstance(*this)));
	Instance->Custom = sol::table(_Lua, sol::create);
	return Instance;
}
//---------------------------------------------------------------------

bool CScriptedAbility::IsAvailable(const CGameSession& Session, const CInteractionContext& Context) const
{
	return !_FnIsAvailable || Scripting::LuaCall(_FnIsAvailable, Context);
}
//---------------------------------------------------------------------

bool CScriptedAbility::IsTargetValid(const CGameSession& Session, U32 Index, const CInteractionContext& Context) const
{
	return Scripting::LuaCall(_FnIsTargetValid, Index, Context);
}
//---------------------------------------------------------------------

bool CScriptedAbility::Execute(CGameSession& Session, CInteractionContext& Context) const
{
	if (Context.Actors.empty()) return false;

	auto pWorld = Session.FindFeature<CGameWorld>();
	if (!pWorld) return false;

	if (_FnPrepare)
	{
		// Filter actors, adjust ability params etc (like +2 bonus in skill for each helping actor)
		NOT_IMPLEMENTED;
		//Scripting::LuaCall(_FnPrepare, ...);
	}

	Context.Commands.clear();
	Context.Commands.reserve(Context.Actors.size());

	UPTR Issued = 0;
	for (HEntity ActorID : Context.Actors)
	{
		Context.Commands.push_back(PushStandardExecuteAction(*pWorld, ActorID, Context));
		if (Context.Commands.back()) ++Issued;
	}

	return Issued > 0;
}
//---------------------------------------------------------------------

bool CScriptedAbility::GetZones(const Game::CGameSession& Session, const CAbilityInstance& Instance, std::vector<const CZone*>& Out) const
{
	return Scripting::LuaCall(_FnGetZones, static_cast<const CScriptedAbilityInstance&>(Instance), Out);
}
//---------------------------------------------------------------------

bool CScriptedAbility::GetFacingParams(const Game::CGameSession& Session, const CAbilityInstance& Instance, CFacingParams& Out) const
{
	if (!_FnGetFacingParams) return false;
	Scripting::LuaCall(_FnGetFacingParams, static_cast<const CScriptedAbilityInstance&>(Instance), Out);
	return Out.Mode != EFacingMode::None;
}
//---------------------------------------------------------------------

void CScriptedAbility::OnStart(Game::CGameSession& Session, CAbilityInstance& Instance) const
{
	Scripting::LuaCall(_FnOnStart, static_cast<const CScriptedAbilityInstance&>(Instance));
}
//---------------------------------------------------------------------

AI::ECommandStatus CScriptedAbility::OnUpdate(Game::CGameSession& Session, CAbilityInstance& Instance) const
{
	auto UpdateResult = _FnOnUpdate(static_cast<const CScriptedAbilityInstance&>(Instance));
	if (!UpdateResult.valid())
	{
		::Sys::Error(UpdateResult.get<sol::error>().what());
		return AI::ECommandStatus::Failed;
	}
	else if (UpdateResult.get_type() != sol::type::number)
	{
		// Enums are represented as numbers in Sol. Also allow to return nil as an alias for Active.
		//!!!TODO: fmtlib and variadic args in assertion macros!
		n_assert2_dbg(UpdateResult.get_type() == sol::type::none, "Unexpected return type from lua OnUpdate");
		return AI::ECommandStatus::Running;
	}

	return UpdateResult;
}
//---------------------------------------------------------------------

void CScriptedAbility::OnEnd(Game::CGameSession& Session, CAbilityInstance& Instance, AI::ECommandStatus Status) const
{
	Scripting::LuaCall(_FnOnEnd, static_cast<const CScriptedAbilityInstance&>(Instance), Status);
}
//---------------------------------------------------------------------

}
