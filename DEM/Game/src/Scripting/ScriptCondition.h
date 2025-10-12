#pragma once
#include <Scripting/Condition.h>
#include <Scripting/SolGame.h>

// A single C++ class for any scripted condition objects

namespace DEM::Game
{

class CScriptCondition : public ICondition
{
protected:

	// This is not state, this is session-level logic
	sol::function _FnEvaluate;
	sol::function _FnGetText;
	sol::function _FnSubscribeRelevantEvents;

public:

	CScriptCondition(sol::table ScriptAsset);

	virtual bool Evaluate(const CConditionContext& Ctx) const override;
	virtual void GetText(std::string& Out, const CConditionContext& Ctx) const override;
	virtual void SubscribeRelevantEvents(std::vector<Events::CConnection>& OutSubs, const CConditionContext& Ctx, const FEventCallback& Callback) const override;
};

}
