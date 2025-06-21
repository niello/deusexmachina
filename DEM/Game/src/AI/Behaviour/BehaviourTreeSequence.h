#pragma once
#include <AI/Behaviour/BehaviourTreeAsset.h>

// A classical BT sequence (AND)

namespace DEM::AI
{

class CBehaviourTreeSequence : public CBehaviourTreeNodeBase
{
	FACTORY_CLASS_DECL;

public:

	virtual U16 Traverse(U16 PrevIdx, U16 SelfIdx, U16 NextIdx, U16 SkipIdx, EStatus ChildStatus, Game::CGameSession& Session) const override;
};

}
