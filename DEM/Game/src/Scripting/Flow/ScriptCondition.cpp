#include "ScriptCondition.h"

namespace DEM::Flow
{

CScriptCondition::CScriptCondition(sol::table ScriptAsset)
	: _FnEvaluate(ScriptAsset.get<sol::function>("Evaluate"))
	, _FnGetText(ScriptAsset.get<sol::function>("GetText"))
	, _FnRENAME_SOMETHING_ABOUT_SUBSCRIPTIONS(ScriptAsset.get<sol::function>("RENAME_SOMETHING_ABOUT_SUBSCRIPTIONS"))
{
}
//---------------------------------------------------------------------

bool CScriptCondition::Evaluate(const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const
{
	return Scripting::LuaCall(_FnEvaluate, Params, Vars);
}
//---------------------------------------------------------------------

void CScriptCondition::GetText(std::string& Out, const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const
{
	Out.append(Scripting::LuaCall<std::string>(_FnGetText, Params, Vars));
}
//---------------------------------------------------------------------

void CScriptCondition::RENAME_SOMETHING_ABOUT_SUBSCRIPTIONS() const
{
	NOT_IMPLEMENTED;
	Scripting::LuaCall<void>(_FnRENAME_SOMETHING_ABOUT_SUBSCRIPTIONS);
}
//---------------------------------------------------------------------

}
