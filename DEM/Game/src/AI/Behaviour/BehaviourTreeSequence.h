#pragma once
#include <AI/Behaviour/BehaviourTreeAsset.h>

// A classical BT sequence (AND)

namespace DEM::AI
{

class CBehaviourTreeSequence : public CBehaviourTreeNodeBase
{
	FACTORY_CLASS_DECL;

public:

	virtual std::pair<EBTStatus, U16> TraverseFromParent(U16 SelfIdx, U16 SkipIdx, Game::CGameSession&) const override
	{
		// Traversing from above always means starting from the first child, if any, or reporting immediate success
		return { EBTStatus::Succeeded, SelfIdx + 1 };
	}

	virtual std::pair<EBTStatus, U16> TraverseFromChild(U16 SelfIdx, U16 SkipIdx, U16 NextIdx, EBTStatus ChildStatus, Game::CGameSession&) const override
	{
		// If the child succeeded, proceed to the next child or to the subtree end if it was the last child.
		// If the child failed or is running, skip the whole sequence subtree and return to the parent.
		return { ChildStatus, (ChildStatus == EBTStatus::Succeeded) ? NextIdx : SkipIdx };
	}
};

}
