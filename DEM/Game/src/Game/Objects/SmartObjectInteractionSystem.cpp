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
			const CSceneComponent& SceneComponent,
			const AI::CNavAgentComponent* pNavAgent)
	{
		if (!SceneComponent.RootNode) return;

		auto Action = Queue.FindCurrent<SwitchSmartObjectState>();
		if (!Action) return;

		const auto ActionStatus = Queue.GetStatus(Action);
		if (ActionStatus != EActionStatus::Active) return;

		const auto ChildAction = Queue.GetChild(Action);
		const auto ChildActionStatus = Queue.GetStatus(ChildAction);
		if (ChildActionStatus == EActionStatus::Cancelled)
		{
			Queue.SetStatus(Action, EActionStatus::Cancelled);
			return;
		}

		auto pAction = Action.As<SwitchSmartObjectState>();

		auto pSOComponent = World.FindComponent<CSmartObjectComponent>(pAction->_Object);
		if (!pSOComponent)
		{
			Queue.SetStatus(Action, EActionStatus::Failed);
			return;
		}

		const CSmartObject* pSOAsset = pSOComponent->Asset->GetObject<CSmartObject>();
		if (!pSOAsset)
		{
			Queue.SetStatus(Action, EActionStatus::Failed);
			return;
		}

		const auto& ActorPos = SceneComponent.RootNode->GetWorldPosition();

		// FIXME: fill! Get facing (only if necessary) from interaction params
		const float ActorRadius = 0.f;
		const bool Static = true; // Means that interaction params can't change, inc. position
		const vector3 ObjectPos(128.0f, 43.5f, 115.0f);
		vector3 ActionPos(127.0f, 43.5f, 115.0f);
		vector3 TargetDir = ObjectPos - ActorPos;
		TargetDir.norm();

//========

		//----------- Move

		if (auto pSteerAction = ChildAction.As<AI::Steer>())
		{
			if (ChildActionStatus == EActionStatus::Active)
			{
				//if (Static) return;
				// if Steer optimization will be used for dynamic objects, must update target here
				// and generate Steer or Navigate. But most probably Steer will be used for Static only.
				// NB: Steer can also be used for dynamic if actor can't navigate. So can check this
				// and if nav agent is null then update Steer target without trying to generate Navigate.
				return;
			}
			else if (ChildActionStatus == EActionStatus::Failed)
			{
				Queue.SetStatus(Action, EActionStatus::Failed);
				return;
			}
		}
		else if (auto pNavAction = ChildAction.As<AI::Navigate>())
		{
			if (ChildActionStatus == EActionStatus::Active)
			{
				if (Static)
				{
					// when the path is (re)built, do one time check for closer iact polygon inside the path
					// if found, get new iact target from its region
					// mark this check as done until the path is changed (version counter in nav agent?)
				}
				else
				{
					// get target position from SO asset (for the curr region) and update it in the action
				}
				return;
			}
			else if (ChildActionStatus == EActionStatus::Failed)
			{
				// try to navigate to the point in the next closest region
				// if all are tried and failed, Queue.SetStatus(Action, EActionStatus::Failed);
				return;
			}
		}
		else if (!Static || !ChildAction)
		{
			// get closest target position and region from SO asset
			// if failed, Queue.SetStatus(Action, EActionStatus::Failed);
			// if not already at the dest reach, generate Navigate or Steer (for Static only?) and return
		}

		//----------- Turn

		if (auto pTurnAction = ChildAction.As<AI::Turn>())
		{
			if (ChildActionStatus == EActionStatus::Active)
			{
				if (Static) return;

				// get facing from SO asset for the current region
				// if facing is still required by SO, update it in Turn action and return
			}
			else if (ChildActionStatus == EActionStatus::Failed)
			{
				Queue.SetStatus(Action, EActionStatus::Failed);
				return;
			}
		}
		else if (!Static || !ChildAction)
		{
			// get facing from SO asset for the current region
			// if facing is required by SO and not already looking at that dir, generate Turn action and return
		}

		//----------- Interact

		// ... anim, state transition etc ...


//========

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
		vector3 LookatDir = -SceneComponent.RootNode->GetWorldMatrix().AxisZ();
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
