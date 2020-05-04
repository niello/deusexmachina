#include "NavSystem.h"
#include <Game/GameLevel.h>
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <AI/Navigation/NavigationComponent.h>

namespace DEM::AI
{

void PlanPath(DEM::Game::CGameWorld& World)
{
	World.ForEachEntityWith<CNavigationComponent, DEM::Game::CActionQueueComponent>(
		[](auto EntityID, auto& Entity, CNavigationComponent& Navigation, DEM::Game::CActionQueueComponent* pActions)
	{
		// Check if an actor must navigate now
		auto pNavigateAction = pActions->FindActive<Navigate>();
		if (!pNavigateAction) return;

		//Navigation.x = 0;

		//???can understand that request is the same? or always compare destination? per-frame but still not awfully slow

		// Logic from CNavSystem::SetDestPoint():
		// if new dest is the same as current and state is not Idle, return
		// set new dest
		// if already there, finish Navigate
		// if has prev target and poly not changed, update target in corridor
		// else reset dest poly, and if dest OK but pos invalid, reset pos poly

		// using context from pNavigation, update path planning and finish with setting up a nested action or with removing Navigate
	});
}
//---------------------------------------------------------------------

}
