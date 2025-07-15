#include <Game/ECS/GameWorld.h>
#include <AI/CommandQueueComponent.h>
#include <AI/CommandStackComponent.h>

namespace DEM::AI
{

void ProcessCommandQueue(Game::CGameWorld& World)
{
	World.ForEachEntityWith<CCommandQueueComponent, CCommandStackComponent>([&World](
		auto EntityID, auto& Entity, CCommandQueueComponent& Queue, CCommandStackComponent& Stack)
	{
		// There are actions running, we must wait for them
		if (!Stack.IsEmpty()) return;

		// Nothing to pass to the execution stack
		if (Queue.IsEmpty()) return;

		Stack.PushCommand(Queue.Pop());
	});
}
//---------------------------------------------------------------------

}
