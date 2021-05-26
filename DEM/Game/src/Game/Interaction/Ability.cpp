#include "Ability.h"
#include <Game/Interaction/AbilityInstance.h>
#include <Game/ECS/GameWorld.h>

namespace DEM::Game
{

bool CAbility::PushStandardExecuteAction(CGameWorld& World, HEntity Actor, const CInteractionContext& Context, bool Enqueue, bool PushChild) const
{
	if (auto pQueue = World.FindComponent<CActionQueueComponent>(Actor))
	{
		PAbilityInstance AbilityInstance(n_new(CAbilityInstance(*this)));
		AbilityInstance->Source = Context.Source;
		AbilityInstance->Targets = Context.Targets;

		// TODO: move to queue method?
		if (PushChild && pQueue->GetCurrent()) pQueue->PushOrUpdateChild<ExecuteAbility>(pQueue->GetCurrent(), std::move(AbilityInstance));
		else
		{
			if (!Enqueue) pQueue->Reset();
			pQueue->EnqueueAction<ExecuteAbility>(std::move(AbilityInstance));
		}
		return true;
	}

	return false;
}
//---------------------------------------------------------------------

}
