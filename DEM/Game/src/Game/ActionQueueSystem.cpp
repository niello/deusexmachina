#include <Game/ECS/GameWorld.h>
#include <AI/CommandQueueComponent.h>
#include <AI/CommandStackComponent.h>

namespace DEM::AI
{

void ProcessActionQueue(Game::CGameWorld& World)
{
	World.ForEachEntityWith<CCommandQueueComponent, CCommandStackComponent>([&World](
		auto EntityID, auto& Entity, CCommandQueueComponent& Queue, CCommandStackComponent& Stack)
	{
		// Finished actions must be popped by their systems, we can't do it here
		if (!Stack.IsEmpty()) return;

		// Nothing to pass to the execution stack
		if (Queue.IsEmpty()) return;

		Stack._CommandStack.push_back(Queue.Pop());
	});
}
//---------------------------------------------------------------------

}
