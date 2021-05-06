#include "ScriptedInteraction.h"
#include <Game/Interaction/InteractionContext.h>
#include <System/System.h>

namespace DEM::Game
{

// TODO: common utility function?!
// FIXME: DUPLICATED CODE! See CScriptedAbility!
template<typename... TArgs>
static bool LuaCall(const sol::function& Fn, TArgs&&... Args)
{
	if (!Fn) return false;

	auto Result = Fn(std::forward<TArgs>(Args)...);
	if (!Result.valid())
	{
		sol::error Error = Result;
		::Sys::Error(Error.what());
		return false;
	}

	if (Result.get_type() == sol::type::nil || !Result) return false;

	return true;
}
//---------------------------------------------------------------------

CScriptedInteraction::CScriptedInteraction(const sol::table& Table)
{
	_FnIsAvailable = Table.get<sol::function>("IsAvailable");
	_FnIsTargetValid = Table.get<sol::function>("IsTargetValid");
	_FnNeedMoreTargets = Table.get<sol::function>("NeedMoreTargets");
	_FnExecute = Table.get<sol::function>("Execute");

	_CursorImage = Table.get<std::string>("CursorImage"); //???to method? pass target index?
}
//---------------------------------------------------------------------

bool CScriptedInteraction::IsAvailable(const CInteractionContext& Context) const
{
	return !_FnIsAvailable || LuaCall(_FnIsAvailable, Context);
}
//---------------------------------------------------------------------

bool CScriptedInteraction::IsTargetValid(const CGameSession& Session, U32 Index, const CInteractionContext& Context) const
{
	return LuaCall(_FnIsTargetValid, Index, Context);
}
//---------------------------------------------------------------------

ESoftBool CScriptedInteraction::NeedMoreTargets(const CInteractionContext& Context) const
{
	auto Result = _FnNeedMoreTargets(Context);
	if (!Result.valid())
	{
		sol::error Error = Result;
		::Sys::Error(Error.what());
		return ESoftBool::False;
	}

	if (Result.get_type() == sol::type::boolean) return Result ? ESoftBool::True : ESoftBool::False;

	return Result;
}
//---------------------------------------------------------------------

bool CScriptedInteraction::Execute(CGameSession& Session, CInteractionContext& Context, bool Enqueue) const
{
	return LuaCall(_FnExecute, Context, Enqueue);
}
//---------------------------------------------------------------------

}
