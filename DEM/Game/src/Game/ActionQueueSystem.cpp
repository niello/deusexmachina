#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>

namespace DEM::Game
{

void ProcessActionQueue(CGameWorld& World)
{
	World.ForEachComponent<CActionQueueComponent>([&World](auto EntityID, CActionQueueComponent& Queue)
	{
		// Sub-actions must be processed by systems that pushed them
		if (Queue.GetStackDepth() > 1) return;

		// TODO: could detect dangling 'New' actions which are not processed by any system
		const auto RootAction = Queue.GetRoot();
		const auto RootStatus = Queue.GetStatus(RootAction);
		switch (RootStatus)
		{
			case EActionStatus::NotQueued:
			case EActionStatus::Succeeded:
			{
				if (RootAction)
					::Sys::Log((std::to_string(EntityID.Raw) + ": " + RootAction.Get()->GetClassName() + " action succeeded\n").c_str());
				Queue.RunNextAction();
				break;
			}
			case EActionStatus::Failed:
			case EActionStatus::Cancelled:
			{
				::Sys::Log((std::to_string(EntityID.Raw) + ": " + RootAction.Get()->GetClassName() + " action failed or was cancelled\n").c_str());
				Queue.Reset(RootStatus);
				break;
			}
		}
	});
}
//---------------------------------------------------------------------

}
