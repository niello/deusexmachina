#include "ScriptCondition.h"
#include <Events/Connection.h>

namespace DEM::Flow
{

CScriptCondition::CScriptCondition(sol::table ScriptAsset)
	: _FnEvaluate(ScriptAsset.get<sol::function>("Evaluate"))
	, _FnGetText(ScriptAsset.get<sol::function>("GetText"))
	, _FnSubscribeRelevantEvents(ScriptAsset.get<sol::function>("SubscribeRelevantEvents"))
{
}
//---------------------------------------------------------------------

bool CScriptCondition::Evaluate(const CConditionContext& Ctx) const
{
	return Scripting::LuaCall(_FnEvaluate, Ctx.Condition.Params, Ctx.pVars);
}
//---------------------------------------------------------------------

void CScriptCondition::GetText(std::string& Out, const CConditionContext& Ctx) const
{
	Out.append(Scripting::LuaCall<std::string>(_FnGetText, Ctx.Condition.Params, Ctx.pVars));
}
//---------------------------------------------------------------------

void CScriptCondition::SubscribeRelevantEvents(std::vector<Events::CConnection>& OutSubs, const CConditionContext& Ctx, const FEventCallback& Callback) const
{
	if (!Callback) return;

	Scripting::LuaCall<void>(_FnSubscribeRelevantEvents, OutSubs, Ctx.Condition.Params, Ctx.pVars, [Callback](sol::stack_object Arg)
	{
		if (Arg.is<sol::nil_t>())
		{
			std::unique_ptr<Game::CGameVarStorage> NoVars;
			Callback(NoVars);
		}
		else if (Arg.is<std::unique_ptr<Game::CGameVarStorage>>())
		{
			Callback(Arg.as<std::unique_ptr<Game::CGameVarStorage>&>());
		}
		else
		{
			::Sys::Error("SubscribeRelevantEvents Lua callback must be called with no arg, nil or a result of CGameVarStorage.new_unique()");
		}
	});
}
//---------------------------------------------------------------------

}
