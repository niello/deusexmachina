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

		auto Action = Queue.FindActive<SwitchSmartObjectState>();
		auto pAction = Action.As<SwitchSmartObjectState>();
		if (!pAction) return;

		const auto ChildAction = Queue.GetChild(Action);
		const auto ChildActionStatus = Queue.GetStatus(ChildAction);
		if (ChildActionStatus == EActionStatus::Failed || ChildActionStatus == EActionStatus::Cancelled)
		{
			// FIXME: if Navigate failed, can retry with another interaction region, until all are tried
			Queue.SetStatus(Action, ChildActionStatus);
			return;
		}

		auto pSOComponent = World.FindComponent<CSmartObjectComponent>(pAction->_Object);
		if (!pSOComponent) return;
		const CSmartObject* pSOAsset = pSOComponent->Asset->GetObject<CSmartObject>();
		if (!pSOAsset) return;

		const auto& ActorPos = pSceneComponent->RootNode->GetWorldPosition();

		// FIXME: fill!
		const float ActorRadius = 0.f;
		// TODO: fill Action.Pos with curr actor pos!
		// TODO: actor anim and/or state
		//???!!!need interaction ID in params?!

		const vector3 ObjectPos(128.0f, 43.5f, 115.0f);
		vector3 ActionPos(127.0f, 43.5f, 115.0f);

		// FIXME: get (only if facing is necessary) from interaction params
		vector3 TargetDir = ObjectPos - ActorPos;
		TargetDir.norm();

		if (auto pNavAction = ChildAction.As<AI::Navigate>())
		{
			// Get target position from cache
			// If failed, try the next closest region (just select second closest now)
		}
		else
		{
			// FIXME: need to refresh if already turning? or maybe finish turning and then validate again?
			// FIXME: where to get interaction name? Request Open iact instead of Opened state? Store both IDs?
			//???instead of returning all this data, return region indexand pos, and facing & anim get from region index when needed?
			std::optional<float> FaceDir;
			if (!pSOAsset->GetInteractionParams(CStrID("Open"), ActorRadius, ActionPos, FaceDir))
			{
				Queue.SetStatus(Action, EActionStatus::Failed);
				return;
			}

			// TODO: calc TargetDir from FaceDir
		}

		if (vector3::SqDistance2D(ActionPos, ActorPos) >= AI::Steer::SqLinearTolerance)
		{
			if (pNavAgent)
			{
				const float Extents[3] = { pNavAgent->Radius, pNavAgent->Height, pNavAgent->Radius };
				float NearestPos[3];

				// FIXME: if static, get poly along with point from object interaction params
				dtPolyRef ObjPolyRef = 0;
				pNavAgent->pNavQuery->findNearestPoly(ActionPos.v, Extents, pNavAgent->Settings->GetQueryFilter(), &ObjPolyRef, NearestPos);

				// FIXME: use pNavAgent->Corridor.getFirstPoly() instead! Offmesh can break this now!
				dtPolyRef AgentPolyRef = 0;
				pNavAgent->pNavQuery->findNearestPoly(ActorPos.v, Extents, pNavAgent->Settings->GetQueryFilter(), &AgentPolyRef, NearestPos);

				if (ObjPolyRef != AgentPolyRef)
				{
					// To avoid possible parent path invalidation, could try to optimize with:
					//Agent.pNavQuery->findLocalNeighbourhood
					//Agent.pNavQuery->moveAlongSurface
					//Agent.pNavQuery->raycast

					Queue.PushOrUpdateChild<AI::Navigate>(Action, ActionPos, 0.f);
					return;
				}
			}

			Queue.PushOrUpdateChild<AI::Steer>(Action, ActionPos, ObjectPos, 0.f);
			return;
		}

		//???update only if there is no Turn already active?
		vector3 LookatDir = -pSceneComponent->RootNode->GetWorldMatrix().AxisZ();
		LookatDir.norm();
		const float Angle = vector3::Angle2DNorm(LookatDir, TargetDir);
		if (std::fabsf(Angle) >= DEM::AI::Turn::AngularTolerance)
		{
			Queue.PushOrUpdateChild<AI::Turn>(Action, TargetDir);
			return;
		}

		pSOComponent->RequestedState = pAction->_State;
		pSOComponent->Force = pAction->_Force;

		// TODO: start actor animation and/or state switching, wait for its end before setting Succeeded status!
		Queue.SetStatus(Action, EActionStatus::Succeeded);
	});
}
//---------------------------------------------------------------------

}
