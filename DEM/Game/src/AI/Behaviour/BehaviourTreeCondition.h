#pragma once
#include <AI/Behaviour/BehaviourTreeAsset.h>

// A BT decorator that checks a condition and either passes execution to its child or returns failure to the parent

namespace DEM::AI
{

class CBehaviourTreeCondition : public CBehaviourTreeNodeBase
{
	FACTORY_CLASS_DECL;

public:

	virtual U16 Traverse(U16 PrevIdx, U16 SelfIdx, U16 NextIdx, U16 SkipIdx, EStatus ChildStatus) const override;
};

}
