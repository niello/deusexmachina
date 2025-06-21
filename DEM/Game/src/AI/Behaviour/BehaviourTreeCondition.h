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
	virtual U16  Traverse(U16 PrevIdx, U16 SelfIdx, U16 NextIdx, U16 SkipIdx, EStatus ChildStatus, Game::CGameSession& Session) const override;
};

}
