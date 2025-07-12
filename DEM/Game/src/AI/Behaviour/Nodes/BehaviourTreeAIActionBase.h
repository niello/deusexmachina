#pragma once
#include <AI/Behaviour/BehaviourTreeAsset.h>

// A leaf BT action that executes an ability by ID
// FIXME: should not be needed when this utility logic will be encapsulated in a new future-based action handling system!

namespace DEM::AI
{
class CCommandFuture;
enum class ECommandStatus : U8;

class CBehaviourTreeAIActionBase : public CBehaviourTreeNodeBase
{
protected:

	static EBTStatus CommandStatusToBTStatus(AI::ECommandStatus Status);
	EBTStatus        GetActionStatus(const CBehaviourTreeContext& Ctx, const CCommandFuture& Cmd) const;
	void             CancelAction(const CBehaviourTreeContext& Ctx, const CCommandFuture& Cmd) const;
};

}
