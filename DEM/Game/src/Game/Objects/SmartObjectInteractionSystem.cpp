#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <Game/ECS/Components/SceneComponent.h>
#include <AI/Navigation/NavAgentComponent.h>
#include <AI/Movement/SteerAction.h>
#include <Game/Objects/SmartObjectComponent.h>
#include <Game/Objects/SmartObject.h>

namespace DEM::Game
{

void InteractWithSmartObjects(CGameWorld& World)
{
	World.ForEachEntityWith<CActionQueueComponent, const CSceneComponent, const AI::CNavAgentComponent*>(
		[&World](auto EntityID, auto& Entity,
			CActionQueueComponent& Queue,
			const CSceneComponent* pSceneComponent,
			const AI::CNavAgentComponent* pNavAgent)
	{
		if (!pSceneComponent->RootNode) return;

		auto pAction = Queue.FindActive<SwitchSmartObjectState>();
		if (!pAction) return;

		/*
		// Check if action or sub-action is stopped
		if (pQueue->GetStatus() == DEM::Game::EActionStatus::Failed || pQueue->GetStatus() == DEM::Game::EActionStatus::Cancelled)
		{
			// If sub-action failed or cancelled, finish with the same result
			// TODO: improve API!
			Queue.FinalizeActiveAction(*pAction, pQueue->GetStatus());
			Queue.RemoveAction(*pAction);
			return;
		}

		// Check if this action succeeded
		if (pQueue->GetActiveStackTop() == pAction && pQueue->GetStatus() == DEM::Game::EActionStatus::Succeeded)
		{
			Queue.RemoveAction(*pAction);
			return;
		}

		// If we didn't finish but no sub-action is active, try to generate it
		if (pQueue->GetActiveStackTop() == pAction || pQueue->GetStatus() == DEM::Game::EActionStatus::Succeeded)
		{
			//
		}
		*/

		//const float ActorRadius = 0.f; // TODO: fill!
		// TODO: fill Action.Pos with curr actor pos!
		// TODO: actor anim and/or state
		//???!!!need interaction ID in params?!
		//if (!pSOAsset->GetInteractionParams(CStrID(_Name.c_str()), ActorRadius, pAction->Pos, pAction->FaceDir)) return false;

		// if not the same poly, add or update Navigate (can try to raycast but require all polys on the way to be Steer-able
		// if not the same position (what tolerance, from Steer?), add or update Steer
		// if not required facing, add or update Face
		// if facing is OK too, start actor animation and request smart object state switching

		const vector3 ObjectPos(128.0f, 43.5f, 115.0f);
		const vector3 ActionPos(127.0f, 43.5f, 115.0f);

		const auto& Pos = pSceneComponent->RootNode->GetWorldPosition();
		if (vector3::SqDistance2D(ActionPos, Pos) >= AI::Steer::SqLinearTolerance)
		{
			if (pNavAgent)
			{
				const float Extents[3] = { pNavAgent->Radius, pNavAgent->Height, pNavAgent->Radius };
				float NearestPos[3];

				dtPolyRef ObjPolyRef = 0;
				pNavAgent->pNavQuery->findNearestPoly(ObjectPos.v, Extents, pNavAgent->Settings->GetQueryFilter(), &ObjPolyRef, NearestPos);

				// FIXME: use pNavAgent->Corridor.getFirstPoly() instead! Offmesh can break this now!
				dtPolyRef AgentPolyRef = 0;
				pNavAgent->pNavQuery->findNearestPoly(Pos.v, Extents, pNavAgent->Settings->GetQueryFilter(), &AgentPolyRef, NearestPos);

				if (ObjPolyRef != AgentPolyRef)
				{
					Queue.PushSubActionForParent<AI::Navigate>(*pAction, ActionPos, 0.f);
					return;
				}
			}

			Queue.PushSubActionForParent<AI::Steer>(*pAction, ActionPos, ObjectPos, 0.f);
			return;
		}

		vector3 LookatDir = -pSceneComponent->RootNode->GetWorldMatrix().AxisZ();
		LookatDir.norm();
		vector3 TargetDir = ObjectPos - Pos;
		TargetDir.norm();
		const float Angle = vector3::Angle2DNorm(LookatDir, TargetDir);
		if (std::fabsf(Angle) >= DEM::AI::Turn::AngularTolerance)
		{
			Queue.PushSubActionForParent<AI::Turn>(*pAction, TargetDir);
			return;
		}

		// Wants to interact with the controller SO. Must check if it is close enough not to navigate to it (can steer).
		//Agent.pNavQuery->findLocalNeighbourhood
		//Agent.pNavQuery->moveAlongSurface
		//Agent.pNavQuery->raycast

		//CSmartObject* pSmart = SOComponent.Asset->GetObject<CSmartObject>();
		//if (!pSmart) return;

		auto pSOComponent = World.FindComponent<CSmartObjectComponent>(pAction->Object);
		if (!pSOComponent) return;
		pSOComponent->RequestedState = pAction->ObjectState;
		pSOComponent->Force = pAction->ForceObjectState;

		//!!!TODO: wait for actor animation or state transition end!
		Queue.FinalizeActiveAction(*pAction, EActionStatus::Succeeded);
	});
}
//---------------------------------------------------------------------

}
