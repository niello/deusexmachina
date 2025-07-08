#include "BehaviourTreeAIActionBase.h"
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
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
	if (auto* pWorld = Ctx.Session.FindFeature<Game::CGameWorld>())
		if (auto* pQueue = pWorld->FindComponent<Game::CActionQueueComponent>(Ctx.ActorID))
			if (pQueue->GetStatus(Action) == Game::EActionStatus::Active)
				pQueue->SetStatus(Action, Game::EActionStatus::Cancelled);
}
//---------------------------------------------------------------------

EBTStatus CBehaviourTreeAIActionBase::GetActionStatus(const CBehaviourTreeContext& Ctx, Game::HAction Action) const
{
	//!!!also has pAIState->_AbilityInstance but there is no status there! At least now.
	if (auto* pWorld = Ctx.Session.FindFeature<Game::CGameWorld>())
		if (auto* pQueue = pWorld->FindComponent<Game::CActionQueueComponent>(Ctx.ActorID))
			return ActionStatusToBTStatus(pQueue->GetStatus(Action));

	return EBTStatus::Succeeded;
}
//---------------------------------------------------------------------

}
