#pragma once
#include <AI/Behaviour/BehaviourTreeAsset.h>

// A classical BT selector (OR)

namespace DEM::AI
{

class CBehaviourTreeSelector : public CBehaviourTreeNodeBase
{
	FACTORY_CLASS_DECL;

public:

	virtual std::pair<EBTStatus, U16> TraverseFromParent(U16 SelfIdx, U16 SkipIdx, const CBehaviourTreeContext&) const override
	{
		// Traversing from above always means starting from the first child, if any, or reporting immediate success
		return { EBTStatus::Succeeded, SelfIdx + 1 };
	}

	virtual std::pair<EBTStatus, U16> TraverseFromChild(U16 SelfIdx, U16 SkipIdx, U16 NextIdx, EBTStatus ChildStatus, const CBehaviourTreeContext&) const override
	{
		// If the child failed, proceed to the next child or to the subtree end if it was the last child.
		// If the child succeeded or is running, skip the whole selector subtree and return to the parent.
		return { ChildStatus, (ChildStatus == EBTStatus::Failed) ? NextIdx : SkipIdx };
	}
};

}
