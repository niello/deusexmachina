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
	return Scripting::LuaCall(_FnEvaluate, Ctx.Condition.Params, Ctx.Vars);
}
//---------------------------------------------------------------------

void CScriptCondition::GetText(std::string& Out, const CConditionContext& Ctx) const
{
	Out.append(Scripting::LuaCall<std::string>(_FnGetText, Ctx.Condition.Params, Ctx.Vars));
}
//---------------------------------------------------------------------

void CScriptCondition::SubscribeRelevantEvents(std::vector<Events::CConnection>& OutSubs, const CConditionContext& Ctx, const std::function<void()>& Callback) const
{
	if (Callback) Scripting::LuaCall<void>(_FnSubscribeRelevantEvents, OutSubs, Ctx.Condition.Params, Ctx.Vars, [Callback](sol::table Params) { Callback(); });
}
//---------------------------------------------------------------------

}
