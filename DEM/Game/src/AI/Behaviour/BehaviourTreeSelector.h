#pragma once
#include <AI/Behaviour/BehaviourTreeAsset.h>

// A classical BT selector (OR)

namespace DEM::AI
{

class CBehaviourTreeSelector : public CBehaviourTreeNodeBase
{
	FACTORY_CLASS_DECL;

public:

	virtual U16 Traverse(U16 PrevIdx, U16 SelfIdx, U16 NextIdx, U16 SkipIdx, EStatus ChildStatus) const override;
};

}
