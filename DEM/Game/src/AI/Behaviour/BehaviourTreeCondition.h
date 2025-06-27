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

	// TODO: subscribe condition events (pCondition->SubscribeRelevantEvents) on player start
	//!!!FIXME: here and in quests must ensure that composite conditions subscribe correctly! now it seems that they don't!

	virtual void Init(const Data::CParams* pParams) override;

	virtual void                      OnTreeStarted(U16 SelfIdx, std::vector<Events::CConnection>& OutSubs, const CBehaviourTreeContext& Ctx) const override;
	virtual std::pair<EBTStatus, U16> TraverseFromParent(U16 SelfIdx, U16 SkipIdx, const CBehaviourTreeContext& Ctx) const override;

	virtual std::pair<EBTStatus, U16> TraverseFromChild(U16 SelfIdx, U16 SkipIdx, U16 NextIdx, EBTStatus ChildStatus, const CBehaviourTreeContext&) const override
	{
		// When the child returns, simply propagate its result up
		return { ChildStatus, NextIdx };
	}
};

}
