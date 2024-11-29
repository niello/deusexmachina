#pragma once
#include <Scripting/Flow/Condition.h>
#include <Scripting/SolGame.h>

// A single C++ class for any scripted condition objects

namespace DEM::Flow
{

class CScriptCondition : public ICondition
{
protected:

	// This is not state, this is session-level logic
	sol::function _FnEvaluate;
	sol::function _FnGetText;
	sol::function _FnRENAME_SOMETHING_ABOUT_SUBSCRIPTIONS;

public:

	CScriptCondition(sol::table ScriptAsset);

	virtual bool Evaluate(const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const override;
	virtual void GetText(std::string& Out, const Data::PParams& Params, Game::CGameSession& Session, const CFlowVarStorage& Vars) const override;
	virtual void RENAME_SOMETHING_ABOUT_SUBSCRIPTIONS() const override;
};

}
