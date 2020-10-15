#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <Game/Objects/SmartObjectComponent.h>
#include <Game/Objects/SmartObject.h>

namespace DEM::Game
{

void InteractWithSmartObjects(CGameWorld& World)
{
	World.ForEachEntityWith<CActionQueueComponent>(
		[&World](auto EntityID, auto& Entity, CActionQueueComponent& Queue)
	{
		auto pAction = Queue.FindActive<SwitchSmartObjectState>();
		if (!pAction) return;

		//!!!???only if no sub-action?!
		//const float ActorRadius = 0.f; // TODO: fill!
		// TODO: fill Action.Pos with curr actor pos!
		// TODO: actor anim and/or state
		//???!!!need interaction ID in params?!
		//if (!pSOAsset->GetInteractionParams(CStrID(_Name.c_str()), ActorRadius, pAction->Pos, pAction->FaceDir)) return false;

		// if not the same poly, add or update Navigate (can try to raycast but require all polys on the way to be Steer-able
		// if not the same position (what tolerance, from Steer?), add or update Steer
		// if not required facing, add or update Face
		// if facing is OK too, start actor animation and request smart object state switching

		//CSmartObject* pSmart = SOComponent.Asset->GetObject<CSmartObject>();
		//if (!pSmart) return;

		auto pSOComponent = World.FindComponent<CSmartObjectComponent>(pAction->Object);
		if (!pSOComponent) return;
		pSOComponent->RequestedState = pAction->ObjectState;
		pSOComponent->Force = pAction->ForceObjectState;
	});
}
//---------------------------------------------------------------------

}
