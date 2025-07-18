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
	std::vector<CStrID>  _UsedBBKeys;
	bool                 _OverrideLowerPriority = true;

public:

	virtual void Init(const Data::CParams* pParams) override;

	virtual void                      OnTreeStarted(U16 SelfIdx, CBehaviourTreePlayer& Player) const override;
	virtual std::pair<EBTStatus, U16> TraverseFromParent(U16 SelfIdx, U16 SkipIdx, const CBehaviourTreeContext& Ctx) const override;
};

}
