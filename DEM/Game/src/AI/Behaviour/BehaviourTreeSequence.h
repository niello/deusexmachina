#pragma once
#include <AI/Behaviour/BehaviourTreeAsset.h>

// A fork action that serves as a hub point for one or more incoming links that share further flow directions.
// It is also capable of selecting a random next link and can therefore serve as a random fork.

namespace DEM::AI
{

class CBehaviourTreeSequence : public CBehaviourTreeNodeBase
{
	FACTORY_CLASS_DECL;

public:

	virtual U16 Traverse(U16 PrevIdx, U16 SelfIdx, U16 NextIdx, U16 SkipIdx, EStatus ChildStatus) const override;
};

}
