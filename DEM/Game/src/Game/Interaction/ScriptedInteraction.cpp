#include "ScriptedInteraction.h"
#include <Game/Interaction/InteractionContext.h>
#include <System/System.h>

namespace DEM::Game
{

// TODO: common utility function?!
// FIXME: DUPLICATED CODE! See CScriptedAbility! Use CRTP?
template<typename... TArgs>
static bool LuaCall(const sol::function& Fn, TArgs&&... Args)
{
	if (!Fn) return false;

	auto Result = Fn(std::forward<TArgs>(Args)...);
	if (!Result.valid())
	{
		::Sys::Error(Result.get<sol::error>().what());
		return false;
	}

	const auto Type = Result.get_type();
	return Type != sol::type::nil && Type != sol::type::none && Result;
}
//---------------------------------------------------------------------

CScriptedInteraction::CScriptedInteraction(const sol::table& Table)
{
	_FnIsAvailable = Table.get<sol::function>("IsAvailable");
	_FnIsTargetValid = Table.get<sol::function>("IsTargetValid");
	_FnExecute = Table.get<sol::function>("Execute");

	_CursorImage = Table.get_or<std::string>("CursorImage", {}); //???to method? pass target index?
	_MandatoryTargets = Table.get_or<U8>("MandatoryTargets", 1);
	_OptionalTargets = Table.get_or<U8>("OptionalTargets", 0);
}
//---------------------------------------------------------------------

bool CScriptedInteraction::IsAvailable(const CGameSession& Session, const CInteractionContext& Context) const
{
	return !_FnIsAvailable || LuaCall(_FnIsAvailable, Context);
}
//---------------------------------------------------------------------

bool CScriptedInteraction::IsTargetValid(const CGameSession& Session, U32 Index, const CInteractionContext& Context) const
{
	return LuaCall(_FnIsTargetValid, Index, Context);
}
//---------------------------------------------------------------------

bool CScriptedInteraction::Execute(CGameSession& Session, CInteractionContext& Context, bool Enqueue, bool PushChild) const
{
	return LuaCall(_FnExecute, Context, Enqueue, PushChild);
}
//---------------------------------------------------------------------

}
