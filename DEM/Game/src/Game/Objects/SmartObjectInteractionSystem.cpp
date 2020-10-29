#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <Game/ECS/Components/SceneComponent.h>
#include <AI/Navigation/NavAgentComponent.h>
#include <AI/Movement/SteerAction.h>
#include <Game/Objects/SmartObjectComponent.h>
#include <Game/Objects/SmartObject.h>
#include <DetourCommon.h>

namespace DEM::Game
{

static inline float SqDistanceToInteractionZone(const vector3& Pos, const CInteractionZone& Zone, UPTR& OutSegment, float& OutT)
{
	const auto VertexCount = Zone.Vertices.size();
	if (VertexCount == 0) return std::numeric_limits<float>().max();
	else if (VertexCount == 1) return vector3::SqDistance(Pos, Zone.Vertices[0]);
	else if (VertexCount == 2) return dtDistancePtSegSqr2D(Pos.v, Zone.Vertices[0].v, Zone.Vertices[1].v, OutT);
	else
	{
		if (Zone.Closed)
		{
			int i, j;
			bool Inside = false;
			float MinSqDistance = std::numeric_limits<float>().max();
			for (i = 0, j = VertexCount - 1; i < static_cast<int>(VertexCount); j = i++)
			{
				const auto& vi = Zone.Vertices[i];
				const auto& vj = Zone.Vertices[j];
				if (((vi.z > Pos.z) != (vj.z > Pos.z)) &&
					(Pos.x < (vj.x - vi.x) * (Pos.z - vi.z) / (vj.z - vi.z) + vi.x))
					Inside = !Inside;

				float t;
				const float SqDistance = dtDistancePtSegSqr2D(Pos.v, vj.v, vi.v, t);
				if (SqDistance < MinSqDistance)
				{
					MinSqDistance = SqDistance;
					OutSegment = i;
					OutT = t;
				}
			}

			if (Inside)
			{
				OutSegment = VertexCount;
				return 0.f;
			}

			return MinSqDistance;
		}
		else
		{
			float MinSqDistance = std::numeric_limits<float>().max();
			for (UPTR i = 0; i < VertexCount - 1; ++i)
			{
				float t;
				const float SqDistance = dtDistancePtSegSqr2D(Pos.v, Zone.Vertices[0].v, Zone.Vertices[1].v, t);
				if (SqDistance < MinSqDistance)
				{
					MinSqDistance = SqDistance;
					OutSegment = i;
					OutT = t;
				}
			}

			return MinSqDistance;
		}
	}
}
//---------------------------------------------------------------------

static inline vector3 PointInInteractionZone(const vector3& Pos, const CInteractionZone& Zone, UPTR Segment, float t)
{
	const auto VertexCount = Zone.Vertices.size();
	if (VertexCount == 0) return vector3::Zero;
	else if (VertexCount == 1) return Zone.Vertices[0];
	else if (VertexCount == 2) return vector3::lerp(Zone.Vertices[0], Zone.Vertices[1], t);
	else if (Segment == VertexCount) return Pos;
	else return vector3::lerp(Zone.Vertices[Segment], Zone.Vertices[(Segment + 1) % VertexCount], t);
}
//---------------------------------------------------------------------

void InteractWithSmartObjects(CGameWorld& World)
{
	World.ForEachEntityWith<CActionQueueComponent, const CSceneComponent, const AI::CNavAgentComponent*>(
		[&World](auto EntityID, auto& Entity,
			CActionQueueComponent& Queue,
			const CSceneComponent& ActorSceneComponent,
			const AI::CNavAgentComponent* pNavAgent)
	{
		if (!ActorSceneComponent.RootNode) return;

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
		if (!pSOComponent || !pSOComponent->Asset)
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

		auto pSOSceneComponent = World.FindComponent<CSceneComponent>(pAction->_Object);
		if (!pSOSceneComponent || !pSOSceneComponent->RootNode)
		{
			Queue.SetStatus(Action, EActionStatus::Failed);
			return;
		}

		const auto& ActorPos = ActorSceneComponent.RootNode->GetWorldPosition();

		// FIXME: fill! Get facing (only if necessary) from interaction params
		const float ActorRadius = 0.f;
		const bool Static = pSOAsset->IsStatic();
		const vector3 ObjectPos(128.0f, 43.5f, 115.0f);
		vector3 ActionPos(127.0f, 43.5f, 115.0f);
		vector3 TargetDir = ObjectPos - ActorPos;
		TargetDir.norm();

		// Move to the interaction point

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
					// TODO: could increment path version on replan and recheck, to handle partial path and corridor optimizations!
					if (!pAction->_PathScanned && pNavAgent && pNavAgent->State == AI::ENavigationState::Following)
					{
						pAction->_PathScanned = true;

						for (int PolyIdx = 0; PolyIdx < pNavAgent->Corridor.getPathCount(); ++PolyIdx)
						{
							const auto PolyRef = pNavAgent->Corridor.getPath()[PolyIdx];

							//   for i : pSOAsset->GetRegionCount(), get pSOComponent->RegionPolys[i] (sorted)
							//     if region was not tried and poly is in RegionPolys (binary search)
							//       pAction->_RegionIndex = curr region
							//       pAction->_TriedRegions |= (1 << curr region)
							//       set point from this region on this poly as a target, no replanning will happen
						}
					}
				}
				else
				{
					// get target position from SO asset (for the curr region) and update it in the action
				}
				return;
			}
			else if (ChildActionStatus == EActionStatus::Failed)
			{
				// try to navigate to the point in the next closest region (static) or any region (dynamic)
				// if all are tried and failed, Queue.SetStatus(Action, EActionStatus::Failed);
				return;
			}
		}
		else if (!Static || !ChildAction)
		{
			matrix44 WorldToSmartObject;
			pSOSceneComponent->RootNode->GetWorldMatrix().invert_simple(WorldToSmartObject);
			const vector3 SOSpaceActorPos = WorldToSmartObject.transform_coord(ActorPos);

			// for each interaction zone (for current iact) not tried yet
			//   find distance from pos to point / segment / polygonal chain / closed polygon
			// get closest zone index
			// if Static, mark zone as tried

			// find closest pos to the actor in the closest zone (can use hint cached above?)
			// convert closest pos to the world space
			// find closest point on poly, center is closest pos, extents is iact R, actor H
			//???use closestPointOnPolyBoundary?
			// if none found or found point is farther than R from closest pos:
			//   either fail this zone or try raycast / findLocalNeighbourhood and test all found polys before failing

			// if failed all zones, Queue.SetStatus(Action, EActionStatus::Failed);

			//!!!must not mark region as tried when not Static!
			std::optional<float> FaceDir;
			if (!pSOAsset->GetInteractionParams(CStrID("Open"), ActorRadius, ActionPos, FaceDir))
			{
				Queue.SetStatus(Action, EActionStatus::Failed);
				return;
			}

			// if not already at the dest reach, generate Navigate or Steer (for Static only?) and return
		}

		// Face interaction direction

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

		// Interact with object

		// ... anim, state transition etc ...

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
		vector3 LookatDir = -ActorSceneComponent.RootNode->GetWorldMatrix().AxisZ();
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
