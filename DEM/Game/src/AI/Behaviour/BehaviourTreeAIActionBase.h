#pragma once
#include <AI/Behaviour/BehaviourTreeAsset.h>

// A leaf BT action that executes an ability by ID
// FIXME: should not be needed when this utility logic will be encapsulated in a new future-based action handling system!

namespace DEM::Game
{
	class HAction;
	enum class EActionStatus : U8;
}

namespace DEM::AI
{

class CBehaviourTreeAIActionBase : public CBehaviourTreeNodeBase
{
protected:

	static EBTStatus ActionStatusToBTStatus(Game::EActionStatus Status);
	EBTStatus        GetActionStatus(const CBehaviourTreeContext& Ctx, Game::HAction Action) const;
	void             CancelAction(const CBehaviourTreeContext& Ctx, Game::HAction Action) const;
};

}
