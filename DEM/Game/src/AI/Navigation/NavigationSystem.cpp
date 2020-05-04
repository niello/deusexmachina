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
		// pActions->FindActive<Navigate>()
		// if not found, return
		// using context from pNavigation, update path planning and finish with setting up a nested action or with removing Navigate
	});
}
//---------------------------------------------------------------------

}
