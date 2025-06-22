#pragma once
#include <AI/Behaviour/BehaviourTreeAsset.h>
#include <Scripting/Flow/Condition.h>

// A BT decorator that checks a condition and either passes execution to its child or returns failure to the parent

namespace DEM::AI
{

class CBehaviourTreeCondition : public CBehaviourTreeNodeBase
{
	FACTORY_CLASS_DECL;

protected:

	Flow::CConditionData _Condition;

public:

	virtual void Init(const Data::CParams* pParams) override;

	virtual std::pair<EBTStatus, U16> TraverseFromParent(U16 SelfIdx, U16 SkipIdx, Game::CGameSession&) const override;

	virtual std::pair<EBTStatus, U16> TraverseFromChild(U16 SelfIdx, U16 SkipIdx, U16 NextIdx, EBTStatus ChildStatus, Game::CGameSession&) const override
	{
		// When the child returns, simply propagate its result up
		return { ChildStatus, NextIdx };
	}
};

}
