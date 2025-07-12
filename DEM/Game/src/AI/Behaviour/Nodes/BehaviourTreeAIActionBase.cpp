#include "BehaviourTreeAIActionBase.h"
#include <Game/ECS/Components/ActionQueueComponent.h>

namespace DEM::AI
{

EBTStatus CBehaviourTreeAIActionBase::ActionStatusToBTStatus(Game::EActionStatus Status)
{
	switch (Status)
	{
		//!!!FIXME: can't know WHY there is no action in queue. Might have succeeded and was reAIActionBased? Or failed? Or cancelled?
		case Game::EActionStatus::NotQueued:
		case Game::EActionStatus::Succeeded:
			return EBTStatus::Succeeded;
		case Game::EActionStatus::Failed:
		case Game::EActionStatus::Cancelled:
			return EBTStatus::Failed;
		default:
			return EBTStatus::Running;
	}
}
//---------------------------------------------------------------------

void CBehaviourTreeAIActionBase::CancelAction(const CBehaviourTreeContext& Ctx, Game::HAction Action) const
{
	if (Ctx.pActuator)
		if (Ctx.pActuator->GetStatus(Action) == Game::EActionStatus::Active)
			Ctx.pActuator->SetStatus(Action, Game::EActionStatus::Cancelled);
}
//---------------------------------------------------------------------

EBTStatus CBehaviourTreeAIActionBase::GetActionStatus(const CBehaviourTreeContext& Ctx, Game::HAction Action) const
{
	if (Ctx.pActuator)
		return ActionStatusToBTStatus(Ctx.pActuator->GetStatus(Action));

	return EBTStatus::Succeeded;
}
//---------------------------------------------------------------------

}
