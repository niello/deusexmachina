#include "BehaviourTreeAIActionBase.h"
#include <AI/Command.h>

namespace DEM::AI
{

EBTStatus CBehaviourTreeAIActionBase::CommandStatusToBTStatus(AI::ECommandStatus Status)
{
	switch (Status)
	{
		case AI::ECommandStatus::Succeeded:
			return EBTStatus::Succeeded;
		case AI::ECommandStatus::Failed:
		case AI::ECommandStatus::Cancelled:
			return EBTStatus::Failed;
		default:
			return EBTStatus::Running;
	}
}
//---------------------------------------------------------------------

void CBehaviourTreeAIActionBase::CancelAction(const CBehaviourTreeContext& Ctx, const CCommandFuture& Cmd) const
{
	if (Cmd.GetStatus() == AI::ECommandStatus::Running)
		Cmd.RequestCancellation();
}
//---------------------------------------------------------------------

EBTStatus CBehaviourTreeAIActionBase::GetActionStatus(const CBehaviourTreeContext& Ctx, const CCommandFuture& Cmd) const
{
	if (Ctx.pActuator)
		return CommandStatusToBTStatus(Cmd.GetStatus());

	return EBTStatus::Succeeded;
}
//---------------------------------------------------------------------

}
