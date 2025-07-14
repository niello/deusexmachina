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

AI::CCommandFuture CAbility::PushStandardExecuteAction(CGameWorld& World, HEntity Actor, const CInteractionContext& Context) const
{
	PAbilityInstance AbilityInstance = CreateInstance(Context);
	if (!AbilityInstance) return {};

	AbilityInstance->Source = Context.Source;
	AbilityInstance->Targets = Context.Targets;

	if (Context.Enqueue)
	{
		auto* pQueue = World.FindOrAddComponent<AI::CCommandQueueComponent>(Actor);
		return pQueue->EnqueueCommand<ExecuteAbility>(std::move(AbilityInstance));
	}
	else
	{
		auto* pActuator = World.FindComponent<AI::CCommandStackComponent>(Actor);
		if (!pActuator) return {};

		if (Context.ResetStack) pActuator->Reset(AI::ECommandStatus::Cancelled);
		return pActuator->PushCommand<ExecuteAbility>(std::move(AbilityInstance));
	}
}
//---------------------------------------------------------------------

}
