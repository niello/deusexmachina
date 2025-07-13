#include "Ability.h"
#include <AI/CommandStackComponent.h>
#include <AI/CommandQueueComponent.h>
#include <Game/Interaction/AbilityInstance.h>
#include <Game/ECS/GameWorld.h>

namespace DEM::Game
{

PAbilityInstance CAbility::CreateInstance(const CInteractionContext& Context) const
{
	return PAbilityInstance(n_new(CAbilityInstance(*this)));
}
//---------------------------------------------------------------------

AI::CCommandFuture CAbility::PushStandardExecuteAction(CGameWorld& World, HEntity Actor, const CInteractionContext& Context, bool Enqueue, bool PushChild) const
{
	PAbilityInstance AbilityInstance = CreateInstance(Context);
	if (!AbilityInstance) return {};

	AbilityInstance->Source = Context.Source;
	AbilityInstance->Targets = Context.Targets;

	if (Enqueue)
	{
		auto* pQueue = World.FindComponent<AI::CCommandQueueComponent>(Actor);
		if (!pQueue) return {};

		return pQueue->EnqueueCommand<ExecuteAbility>(std::move(AbilityInstance));
	}
	else
	{
		auto* pActuator = World.FindComponent<AI::CCommandStackComponent>(Actor);
		if (!pActuator) return {};

		if (!PushChild) pActuator->Reset(AI::ECommandStatus::Cancelled);
		return pActuator->PushCommand<ExecuteAbility>(std::move(AbilityInstance));
	}
}
//---------------------------------------------------------------------

}
