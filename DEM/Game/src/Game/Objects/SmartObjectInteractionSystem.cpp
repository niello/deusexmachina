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

// NB: if closed, must be convex
static float SqDistanceToPolyChain(const vector3& Pos, const float* pVertices, UPTR VertexCount, bool Closed, UPTR& OutSegment, float& OutT)
{
	float MinSqDistance = std::numeric_limits<float>().max();
	if (Closed)
	{
		// NB: only convex polys are supported for now!
		if (dtPointInPolygon(Pos.v, pVertices, VertexCount))
		{
			OutSegment = VertexCount;
			return 0.f;
		}

		// Process implicit closing edge
		MinSqDistance = dtDistancePtSegSqr2D(Pos.v, &pVertices[3 * (VertexCount - 1)], &pVertices[0], OutT);
		OutSegment = VertexCount - 1;
	}

	for (UPTR i = 0; i < VertexCount - 1; ++i)
	{
		float t;
		const float SqDistance = dtDistancePtSegSqr2D(Pos.v, &pVertices[3 * i], &pVertices[3 * (i + 1)], t);
		if (SqDistance < MinSqDistance)
		{
			MinSqDistance = SqDistance;
			OutSegment = i;
			OutT = t;
		}
	}

	return MinSqDistance;
}
//---------------------------------------------------------------------

static inline float SqDistanceToInteractionZone(const vector3& Pos, const CInteractionZone& Zone, UPTR& OutSegment, float& OutT)
{
	const auto VertexCount = Zone.Vertices.size();
	if (VertexCount == 0) return std::numeric_limits<float>().max();
	else if (VertexCount == 1) return vector3::SqDistance2D(Pos, Zone.Vertices[0]);
	else if (VertexCount == 2) return dtDistancePtSegSqr2D(Pos.v, Zone.Vertices[0].v, Zone.Vertices[1].v, OutT);
	else return SqDistanceToPolyChain(Pos, Zone.Vertices.data()->v, VertexCount, Zone.ClosedPolygon, OutSegment, OutT);
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

// NB: OutPos is not changed if function returns false
static bool IsNavPolyInInteractionZone(const CInteractionZone& Zone, const matrix44& ObjectWorldTfm, float* pPolyVerts, int PolyVertCount, vector3& OutPos)
{
	UPTR SegmentIdx = 0;
	float t = 0.f;
	float SqDistance = std::numeric_limits<float>().max();

	const auto SqRadius = Zone.Radius * Zone.Radius;
	const auto VertexCount = Zone.Vertices.size();
	if (VertexCount < 2)
	{
		const vector3 Pos = VertexCount ? ObjectWorldTfm.transform_coord(Zone.Vertices[0]) : ObjectWorldTfm.Translation();
		SqDistance = SqDistanceToPolyChain(Pos, pPolyVerts, PolyVertCount, true, SegmentIdx, t);
	}
	else if (VertexCount == 2)
	{
		// https://www.geometrictools.com/GTE/Mathematics/DistSegmentSegment.h

		//dtIntersectSegSeg2D
		//dtIntersectSegmentPoly2D(
		//	ObjectWorldTfm.transform_coord(Zone.Vertices[0]).v,
		//	ObjectWorldTfm.transform_coord(Zone.Vertices[1]).v,

		// FIXME: IMPLEMENT!
		return false;
	}
	else
	{
		// Can use separating axes to get distances?

		// FIXME: IMPLEMENT!
		return false;
	}

	if (SqDistance > SqRadius) return false;
	dtVlerp(OutPos.v, &pPolyVerts[3 * SegmentIdx], &pPolyVerts[(3 * (SegmentIdx + 1)) % VertexCount], t);
	return true;
}
//---------------------------------------------------------------------

// If new path intersects with another interaction zone, can optimize by navigating to it instead of the original target
static void OptimizeStaticPath(SwitchSmartObjectState& Action, AI::Navigate& NavAction, const matrix44& ObjectWorldTfm, const CSmartObject& SO, const AI::CNavAgentComponent* pNavAgent)
{
	//!!!TODO: instead of _PathScanned could increment path version on replan and recheck,
	//to handle partial path and corridor optimizations!
	if (Action._PathScanned || !Action._AllowedZones || !pNavAgent || pNavAgent->State != AI::ENavigationState::Following) return;

	Action._PathScanned = true;

	const auto ZoneCount = SO.GetInteractionZoneCount();

	float Verts[DT_VERTS_PER_POLYGON * 3];

	// Don't test the last poly, we already navigate to it
	const int PolysToTest = pNavAgent->Corridor.getPathCount() - 1;
	for (int PolyIdx = 0; PolyIdx < PolysToTest; ++PolyIdx)
	{
		const auto PolyRef = pNavAgent->Corridor.getPath()[PolyIdx];
		const dtMeshTile* pTile = nullptr;
		const dtPoly* pPoly = nullptr;
		pNavAgent->pNavQuery->getAttachedNavMesh()->getTileAndPolyByRefUnsafe(PolyRef, &pTile, &pPoly);

		const int PolyVertCount = pPoly->vertCount;
		for (int i = 0; i < PolyVertCount; ++i)
			dtVcopy(&Verts[i * 3], &pTile->verts[pPoly->verts[i] * 3]);

		for (U8 ZoneIdx = 0; ZoneIdx < ZoneCount; ++ZoneIdx)
		{
			if (!((1 << ZoneIdx) & Action._AllowedZones)) continue;
			if (IsNavPolyInInteractionZone(SO.GetInteractionZone(ZoneIdx), ObjectWorldTfm, Verts, PolyVertCount, NavAction._Destination))
			{
				Action._ZoneIndex = ZoneIdx;
				Action._AllowedZones &= ~(1 << ZoneIdx);
				return;
			}
		}
	}
}
//---------------------------------------------------------------------

static bool UpdateMovementSubAction(CActionQueueComponent& Queue, HAction Action, const CSmartObject& SO,
	const AI::CNavAgentComponent* pNavAgent, const matrix44& ObjectWorldTfm, const vector3& ActorPos, bool OnlyCurrZone)
{
	matrix44 WorldToSmartObject;
	ObjectWorldTfm.invert_simple(WorldToSmartObject);
	const vector3 SOSpaceActorPos = WorldToSmartObject.transform_coord(ActorPos);

	auto pAction = Action.As<SwitchSmartObjectState>();
	const auto PrevAllowedZones = pAction->_AllowedZones;

	while (OnlyCurrZone || pAction->_AllowedZones)
	{
		UPTR SegmentIdx = 0;
		float t = 0.f;
		if (OnlyCurrZone)
		{
			SqDistanceToInteractionZone(SOSpaceActorPos, SO.GetInteractionZone(pAction->_ZoneIndex), SegmentIdx, t);
		}
		else
		{
			// Find closest suitable zone
			// NB: could calculate once, sort and then loop over them, but most probably it will be slower than now,
			// because it is expected that almost always the first or at worst the second interaction zone will be selected.
			float MinSqDistance = std::numeric_limits<float>().max();
			const auto ZoneCount = SO.GetInteractionZoneCount();
			for (U8 i = 0; i < ZoneCount; ++i)
			{
				if (!((1 << i) & pAction->_AllowedZones)) continue;

				UPTR CurrS;
				float CurrT;
				const float SqDistance = SqDistanceToInteractionZone(SOSpaceActorPos, SO.GetInteractionZone(i), CurrS, CurrT);
				if (SqDistance < MinSqDistance)
				{
					MinSqDistance = SqDistance;
					SegmentIdx = CurrS;
					t = CurrT;
					pAction->_ZoneIndex = i;
				}
			}
		}

		pAction->_AllowedZones &= ~(1 << pAction->_ZoneIndex);

		const auto& Zone = SO.GetInteractionZone(pAction->_ZoneIndex);

		vector3 ActionPos = ObjectWorldTfm.transform_coord(PointInInteractionZone(SOSpaceActorPos, Zone, SegmentIdx, t));

		const float WorldSqDistance = vector3::SqDistance2D(ActionPos, ActorPos);

		const float Radius = Zone.Radius; //???apply actor radius too?
		const float SqRadius = std::max(Radius * Radius, AI::Steer::SqLinearTolerance);

		// FIXME: what if there is something between ActionPos and ActorPos that blocks an interaction?
		if (WorldSqDistance <= SqRadius) return false;

		ActionPos = vector3::lerp(ActionPos, ActorPos, Radius / n_sqrt(WorldSqDistance));

		if (pNavAgent)
		{
			float NearestPos[3];
			const float Extents[3] = { Radius, pNavAgent->Height, Radius };
			dtPolyRef ObjPolyRef = 0;
			pNavAgent->pNavQuery->findNearestPoly(ActionPos.v, Extents, pNavAgent->Settings->GetQueryFilter(), &ObjPolyRef, NearestPos);
			const float SqDiff = dtVdist2DSqr(ActionPos.v, NearestPos);
			if (!ObjPolyRef || SqDiff > SqRadius)
			{
				// No navigable point found inside a zone
				// NB: this check is simplified and may fail to navigate zones where reachable points exist, so, in order
				// to work, each zone must have at least one navigable point in a zone radius from each base point.
				// TODO: could instead explore all the interaction zone for intersecion with valid navigation polys, but it is slow.
				if (OnlyCurrZone) break;
				continue;
			}
			else if (SqDiff > 0.f) ActionPos = NearestPos;

			// FIXME: use pNavAgent->Corridor.getFirstPoly() instead! Offmesh can break this now!
			const float AgentExtents[3] = { pNavAgent->Radius, pNavAgent->Height, pNavAgent->Radius };
			dtPolyRef AgentPolyRef = 0;
			pNavAgent->pNavQuery->findNearestPoly(ActorPos.v, AgentExtents, pNavAgent->Settings->GetQueryFilter(), &AgentPolyRef, NearestPos);
			//n_assert_dbg(pNavAgent->Corridor.getFirstPoly() == AgentPolyRef);

			if (ObjPolyRef != AgentPolyRef)
			{
				// To avoid possible parent path invalidation, could try to optimize with findLocalNeighbourhood,
				// moveAlongSurface or raycast, but it would require additional logic and complicate navigation.
				Queue.PushOrUpdateChild<AI::Navigate>(Action, ActionPos, 0.f);
				return true;
			}
		}

		Queue.PushOrUpdateChild<AI::Steer>(Action, ActionPos, ObjectWorldTfm.Translation(), 0.f);
		return true;
	}

	// No suitable zones left. Static SO fails, but dynamic still has a chance in subsequent frames.
	if (SO.IsStatic())
		Queue.SetStatus(Action, EActionStatus::Failed);
	else
		pAction->_AllowedZones = PrevAllowedZones;

	return true;
}
//---------------------------------------------------------------------

static bool UpdateFacingSubAction(CActionQueueComponent& Queue, HAction Action, const CSmartObject& SO,
	const matrix44& ObjectWorldTfm, const matrix44& ActorWorldTfm)
{
	auto pAction = Action.As<SwitchSmartObjectState>();
	const auto& Zone = SO.GetInteractionZone(pAction->_ZoneIndex);

	auto It = std::find_if(Zone.Interactions.cbegin(), Zone.Interactions.cend(), [ID = pAction->_Interaction](const auto& Elm)
	{
		return Elm.ID == ID;
	});
	if (It == Zone.Interactions.cend())
	{
		// Should never happen
		Queue.SetStatus(Action, EActionStatus::Failed);
		return false;
	}

	vector3 TargetDir;
	switch (It->FacingMode)
	{
		case EFacingMode::Direction:
		{
			TargetDir = It->FacingDir;
			break;
		}
		case EFacingMode::Point:
		{
			TargetDir = ObjectWorldTfm.transform_coord(It->FacingDir) - ActorWorldTfm.Translation();
			TargetDir.y = 0.f;
			TargetDir.norm();
			break;
		}
		default: return false;
	}

	vector3 LookatDir = -ActorWorldTfm.AxisZ();
	LookatDir.norm();
	const float Angle = vector3::Angle2DNorm(LookatDir, TargetDir);
	if (std::fabsf(Angle) <= DEM::AI::Turn::AngularTolerance) return false;

	Queue.PushOrUpdateChild<AI::Turn>(Action, TargetDir);
	return true;
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

		const auto& ActorWorldTfm = ActorSceneComponent.RootNode->GetWorldMatrix();
		const auto& ActorPos = ActorWorldTfm.Translation();
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
					// FIXME: distance from segment to poly and from poly to poly are not calculated yet, so optimization works only
					// for point/circle zones. Maybe a cheaper way is to UpdateMovementSubAction when enter new poly or by time?
					OptimizeStaticPath(*pAction, *pNavAction, ObjectWorldTfm, *pSOAsset, pNavAgent);
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
				else if (UpdateFacingSubAction(Queue, Action, *pSOAsset, ObjectWorldTfm, ActorWorldTfm)) return;
			}
			else if (ChildActionStatus == EActionStatus::Failed)
			{
				Queue.SetStatus(Action, EActionStatus::Failed);
				return;
			}
		}
		else
		{
			if (UpdateFacingSubAction(Queue, Action, *pSOAsset, ObjectWorldTfm, ActorWorldTfm)) return;
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