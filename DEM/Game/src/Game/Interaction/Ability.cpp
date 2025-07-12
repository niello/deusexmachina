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

bool CAbility::PushStandardExecuteAction(CGameWorld& World, HEntity Actor, const CInteractionContext& Context, bool Enqueue, bool PushChild) const
{
	PAbilityInstance AbilityInstance = CreateInstance(Context);
	if (!AbilityInstance) return false;

	AbilityInstance->Source = Context.Source;
	AbilityInstance->Targets = Context.Targets;

	if (Enqueue)
	{
		auto* pQueue = World.FindComponent<AI::CCommandQueueComponent>(Actor);
		if (!pQueue) return false;

		//???return future?! need empty future for failure or never fails? how to return failure from function that returns future? use optional?
		pQueue->EnqueueCommand<ExecuteAbility>(std::move(AbilityInstance));
	}
	else
	{
		auto* pActuator = World.FindComponent<AI::CCommandStackComponent>(Actor);
		if (!pActuator) return false;

		// TODO: set to root or push to top of the stack based on PushChild
		pActuator->PushOrUpdateChild<ExecuteAbility>(pQueue->GetCurrent(), std::move(AbilityInstance));
	}

	return true;
}
//---------------------------------------------------------------------

}
