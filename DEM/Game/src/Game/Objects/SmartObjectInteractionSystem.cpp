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
		float MinSqDistance = std::numeric_limits<float>().max();
		if (Zone.ClosedPolygon)
		{
			// NB: only convex polys are supported for now!
			if (dtPointInPolygon(Pos.v, Zone.Vertices[0].v, VertexCount))
			{
				OutSegment = VertexCount;
				return 0.f;
			}

			// Process implicit closing edge
			MinSqDistance = dtDistancePtSegSqr2D(Pos.v, Zone.Vertices[VertexCount - 1].v, Zone.Vertices[0].v, OutT);
			OutSegment = VertexCount - 1;
		}

		for (UPTR i = 0; i < VertexCount - 1; ++i)
		{
			float t;
			const float SqDistance = dtDistancePtSegSqr2D(Pos.v, Zone.Vertices[i].v, Zone.Vertices[i + 1].v, t);
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

// NB: non-zero _AllowedZones implied
static vector3 FindLocalInteractionPoint(SwitchSmartObjectState& Action, const CSmartObject& SO, const vector3& ActorLocalPos, bool OnlyCurrZone)
{
	float MinSqDistance = std::numeric_limits<float>().max();
	UPTR SegmentIdx = 0;
	float t = 0.f;

	if (OnlyCurrZone)
	{
		MinSqDistance = SqDistanceToInteractionZone(ActorLocalPos, SO.GetInteractionZone(Action._ZoneIndex), SegmentIdx, t);
	}
	else
	{
		// Find closest suitable zone
		const auto ZoneCount = SO.GetInteractionZoneCount();
		for (U8 i = 0; i < ZoneCount; ++i)
		{
			if (!((1 << i) & Action._AllowedZones)) continue;

			const float SqDistance = SqDistanceToInteractionZone(ActorLocalPos, SO.GetInteractionZone(i), SegmentIdx, t);
			if (SqDistance < MinSqDistance)
			{
				MinSqDistance = SqDistance;
				Action._ZoneIndex = i;
			}
		}
	}

	// For static smart objects try each zone only once
	if (SO.IsStatic()) Action._AllowedZones &= ~(1 << Action._ZoneIndex);

	return PointInInteractionZone(ActorLocalPos, SO.GetInteractionZone(Action._ZoneIndex), SegmentIdx, t);
}
//---------------------------------------------------------------------

//???when dynamic fails? zones aren't marked as failed!
static bool UpdateMovementSubAction(CActionQueueComponent& Queue, HAction Action, const CSmartObject& SO,
	const AI::CNavAgentComponent* pNavAgent, const matrix44& ObjectWorldTfm, const vector3& ActorPos, bool OnlyCurrZone)
{
	auto pAction = Action.As<SwitchSmartObjectState>();
	if (!pAction->_AllowedZones)
	{
		Queue.SetStatus(Action, EActionStatus::Failed);
		return true;
	}

	matrix44 WorldToSmartObject;
	ObjectWorldTfm.invert_simple(WorldToSmartObject);
	const vector3 SOSpaceActorPos = WorldToSmartObject.transform_coord(ActorPos);

	vector3 ActionPos = FindLocalInteractionPoint(*pAction, SO, SOSpaceActorPos, OnlyCurrZone);

	const auto& Zone = SO.GetInteractionZone(pAction->_ZoneIndex);
	const float Radius = Zone.Radius; //???apply actor radius too?
	const float SqRadius = std::max(Radius * Radius, AI::Steer::SqLinearTolerance);

	// FIXME: find closest navigable point, use radius as arrival distance, apply to the last path segment
	//if (SqRadius < MinSqDistance)
	//	ActionPos = vector3::lerp(ActionPos, SOSpaceActorPos, Radius / n_sqrt(MinSqDistance));

	ActionPos = ObjectWorldTfm.transform_coord(ActionPos);

	if (vector3::SqDistance2D(ActionPos, ActorPos) <= SqRadius) return false;

	if (pNavAgent)
	{
		float NearestPos[3];
		const float Extents[3] = { Radius, pNavAgent->Height, Radius };
		dtPolyRef ObjPolyRef = 0;
		pNavAgent->pNavQuery->findNearestPoly(ActionPos.v, Extents, pNavAgent->Settings->GetQueryFilter(), &ObjPolyRef, NearestPos);
		const float SqDiff = dtVdist2DSqr(ActionPos.v, NearestPos);
		if (!ObjPolyRef || SqDiff > SqRadius)
		{
			// No navigable point found
			// FIXME: try the next zone right now!
			// TODO: could explore all the interaction zone for intersecion with valid navigation polys, but it is slow
			//???what if dynamic? discard this zone or fail immediately?
			return true;
		}
		else if (SqDiff > 0.f) ActionPos = NearestPos;

		// FIXME: use pNavAgent->Corridor.getFirstPoly() instead! Offmesh can break this now!
		const float AgentExtents[3] = { pNavAgent->Radius, pNavAgent->Height, pNavAgent->Radius };
		dtPolyRef AgentPolyRef = 0;
		pNavAgent->pNavQuery->findNearestPoly(ActorPos.v, AgentExtents, pNavAgent->Settings->GetQueryFilter(), &AgentPolyRef, NearestPos);
		//n_assert_dbg(pNavAgent->Corridor.getFirstPoly() == AgentPolyRef);

		if (ObjPolyRef != AgentPolyRef)
		{
			// To avoid possible parent path invalidation, could try to optimize with:
			//Agent.pNavQuery->findLocalNeighbourhood
			//Agent.pNavQuery->moveAlongSurface
			//Agent.pNavQuery->raycast
			// but it would require additional logic and complicate navigation.
			Queue.PushOrUpdateChild<AI::Navigate>(Action, ActionPos, 0.f);
			return true;
		}
	}

	Queue.PushOrUpdateChild<AI::Steer>(Action, ActionPos, ObjectWorldTfm.Translation(), 0.f);
	return true;
}
//---------------------------------------------------------------------

// NB: OutPos is not changed if function returns false
static bool IsNavPolyInInteractionZone(const CInteractionZone& Zone, dtPolyRef PolyRef, dtNavMeshQuery& Query, vector3& OutPos)
{
	////!!!calc distance from PolyRef to Zone skeleton!
	//const float SqDistance = 0.f;

	//const float Radius = Zone.Radius; //???apply actor radius too?
	//const float SqRadius = std::max(Radius * Radius, AI::Steer::SqLinearTolerance);
	//return SqDistance <= SqRadius;

	//!!!return closest pos!

	//NOT_IMPLEMENTED;
	return false;
}
//---------------------------------------------------------------------

// If new path intersects with another interaction zone, can optimize by navigating to it instead of the original target
static void OptimizeStaticPath(SwitchSmartObjectState& Action, AI::Navigate& NavAction, const CSmartObject& SO, const AI::CNavAgentComponent* pNavAgent)
{
	//!!!TODO: instead of _PathScanned could increment path version on replan and recheck,
	//to handle partial path and corridor optimizations!
	if (Action._PathScanned || !Action._AllowedZones || !pNavAgent || pNavAgent->State != AI::ENavigationState::Following) return;

	Action._PathScanned = true;

	const auto ZoneCount = SO.GetInteractionZoneCount();

	// Don't test the last poly, we already navigate to it
	const int PolysToTest = pNavAgent->Corridor.getPathCount() - 1;
	for (int PolyIdx = 0; PolyIdx < PolysToTest; ++PolyIdx)
	{
		const auto PolyRef = pNavAgent->Corridor.getPath()[PolyIdx];
		for (U8 ZoneIdx = 0; ZoneIdx < ZoneCount; ++ZoneIdx)
		{
			if (!((1 << ZoneIdx) & Action._AllowedZones)) continue;
			if (IsNavPolyInInteractionZone(SO.GetInteractionZone(ZoneIdx), PolyRef, *pNavAgent->pNavQuery, NavAction._Destination))
			{
				Action._ZoneIndex = ZoneIdx;
				Action._AllowedZones &= ~(1 << ZoneIdx);
				return;
			}
		}
	}
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

		// Cache allowed zones once
		if (!pAction->_AllowedZones)
		{
			const auto ZoneCount = pSOAsset->GetInteractionZoneCount();
			for (U8 i = 0; i < ZoneCount; ++i)
			{
				const auto& Zone = pSOAsset->GetInteractionZone(i);
				for (const auto& Interaction : Zone.Interactions)
				{
					if (Interaction.ID == pAction->_Interaction)
					{
						pAction->_AllowedZones |= (1 << i);
						break;
					}
				}
			}

			if (!pAction->_AllowedZones)
			{
				Queue.SetStatus(Action, EActionStatus::Failed);
				return;
			}
		}

		const auto& ActorPos = ActorSceneComponent.RootNode->GetWorldPosition();
		const auto& ObjectWorldTfm = pSOSceneComponent->RootNode->GetWorldMatrix();

		// Move to the interaction point

		if (auto pSteerAction = ChildAction.As<AI::Steer>())
		{
			if (ChildActionStatus == EActionStatus::Active)
			{
				if (pSOAsset->IsStatic()) return;
				else if (UpdateMovementSubAction(Queue, Action, *pSOAsset, pNavAgent, ObjectWorldTfm, ActorPos, true)) return;
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
				if (pSOAsset->IsStatic())
				{
					OptimizeStaticPath(*pAction, *pNavAction, *pSOAsset, pNavAgent);
					return;
				}
				else
				{
					// Update movement target from the current zone
					if (UpdateMovementSubAction(Queue, Action, *pSOAsset, pNavAgent, ObjectWorldTfm, ActorPos, true)) return;
				}
			}
			else if (ChildActionStatus == EActionStatus::Failed)
			{
				// Try another remaining zones with navigable points one by one, fail if none left
				if (UpdateMovementSubAction(Queue, Action, *pSOAsset, pNavAgent, ObjectWorldTfm, ActorPos, false)) return;
			}
		}
		else if (!pSOAsset->IsStatic() || !ChildAction)
		{
			// Move to the closest zone with navigable point inside, fail if none left
			if (UpdateMovementSubAction(Queue, Action, *pSOAsset, pNavAgent, ObjectWorldTfm, ActorPos, false)) return;
		}

		// Face interaction direction

		if (auto pTurnAction = ChildAction.As<AI::Turn>())
		{
			if (ChildActionStatus == EActionStatus::Active)
			{
				if (pSOAsset->IsStatic()) return;

				// get facing from SO asset for the current region
				// if facing is still required by SO, update it in Turn action and return
			}
			else if (ChildActionStatus == EActionStatus::Failed)
			{
				Queue.SetStatus(Action, EActionStatus::Failed);
				return;
			}
		}
		else
		{
			// get facing from SO asset for the current region
			// if facing is required by SO and not already looking at that dir, generate Turn action and return

			//!!!DBG TMP!
			vector3 TargetDir = ObjectWorldTfm.Translation() - ActorPos;
			TargetDir.norm();

			//???update only if there is no Turn already active?
			vector3 LookatDir = -ActorSceneComponent.RootNode->GetWorldMatrix().AxisZ();
			LookatDir.norm();
			const float Angle = vector3::Angle2DNorm(LookatDir, TargetDir);
			if (std::fabsf(Angle) >= DEM::AI::Turn::AngularTolerance)
			{
				Queue.PushOrUpdateChild<AI::Turn>(Action, TargetDir);
				return;
			}
		}

		// Interact with object

		pSOComponent->RequestedState = pAction->_State;
		pSOComponent->Force = pAction->_Force;

		// TODO: start actor animation and/or state switching, wait for its end before setting Succeeded status!
		Queue.SetStatus(Action, EActionStatus::Succeeded);
	});
}
//---------------------------------------------------------------------

}
