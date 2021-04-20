#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <Game/ECS/Components/SceneComponent.h>
#include <AI/Navigation/NavAgentComponent.h>
#include <AI/AIStateComponent.h>
#include <AI/Movement/SteerAction.h>
#include <Game/Objects/SmartObjectComponent.h>
#include <Game/Objects/SmartObject.h>
#include <DetourCommon.h>

// Ability execution consists of the following steps:
// 1. Approach an interaction point
// 2. Face appropriate direction, if required
// 3. Start ability
// 4. Update ability until done, cancelled or failed
// 5. End ability

namespace DEM::Game
{

// FIXME: distance from segment to poly and from poly to poly are not calculated yet, so optimization works only
// for point/circle zones. Maybe a cheaper way is to update sub-action when enter new poly or by timer?
static bool OptimizeStaticPath(CGameWorld& World, HEntity EntityID, InteractWithSmartObject& Action, AI::Navigate& NavAction, const CSmartObject& SO)
{
	//!!!TODO: instead of _PathScanned could increment path version on replan and recheck,
	//to handle partial path and corridor optimizations!
	if (Action._PathScanned || !Action._AllowedZones) return true;

	auto pSOSceneComponent = World.FindComponent<CSceneComponent>(Action._Object);
	if (!pSOSceneComponent || !pSOSceneComponent->RootNode) return false;
	const auto& ObjectWorldTfm = pSOSceneComponent->RootNode->GetWorldMatrix();

	const auto* pNavAgent = World.FindComponent<AI::CNavAgentComponent>(EntityID);
	if (!pNavAgent || pNavAgent->State != AI::ENavigationState::Following) return true;

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
			const CZone& Zone = SO.GetInteractionZone(ZoneIdx).Zone;
			if (Zone.IntersectsNavPoly(ObjectWorldTfm, Verts, PolyVertCount, NavAction._Destination))
			{
				Action._ZoneIndex = ZoneIdx;
				Action._AllowedZones &= ~(1 << ZoneIdx);
				return true;
			}
		}
	}

	return true;
}
//---------------------------------------------------------------------

static bool GetFacingParams(const CInteractionZone& Zone, CStrID InteractionID, const matrix44& ObjectWorldTfm,
	const vector3& ActorPos, vector3& OutFacingDir, float& OutFacingTolerance)
{
	auto It = std::find_if(Zone.Interactions.cbegin(), Zone.Interactions.cend(), [InteractionID](const auto& Elm)
	{
		return Elm.ID == InteractionID;
	});

	if (It != Zone.Interactions.cend()) // Should always be true
	{
		OutFacingTolerance = It->FacingTolerance;

		switch (It->FacingMode)
		{
			case EFacingMode::Direction:
			{
				OutFacingDir = It->FacingDir;
				return true;
			}
			case EFacingMode::Point:
			{
				OutFacingDir = ObjectWorldTfm.transform_coord(It->FacingDir) - ActorPos;
				OutFacingDir.y = 0.f;
				OutFacingDir.norm();
				return true;
			}
		}
	}

	OutFacingDir = vector3::Zero;
	OutFacingTolerance = AI::Turn::AngularTolerance;
	return false;
}
//---------------------------------------------------------------------

static EActionStatus MoveToTarget(CGameWorld& World, HEntity EntityID, CActionQueueComponent& Queue, HAction Action,
	HAction ChildAction, EActionStatus ChildActionStatus, const CSmartObject& SO)
{
	auto pAction = Action.As<InteractWithSmartObject>();

	bool CheckOnlyCurrentZone = true;
	if (auto pSteerAction = ChildAction.As<AI::Steer>())
	{
		if (ChildActionStatus == EActionStatus::Active)
		{
			// No need to update destination if the target is static
			if (SO.IsStatic()) return EActionStatus::Active;
		}
		else return ChildActionStatus; // May be only Failed or Succeeded here
	}
	else if (auto pNavAction = ChildAction.As<AI::Navigate>())
	{
		if (ChildActionStatus == EActionStatus::Active)
		{
			if (SO.IsStatic())
			{
				// If new path intersects with another interaction zone, can optimize by navigating to it instead of the original target
				const bool Ok = OptimizeStaticPath(World, EntityID, *pAction, *pNavAction, SO);
				return Ok ? EActionStatus::Active : EActionStatus::Failed;
			}
		}
		else if (ChildActionStatus == EActionStatus::Failed)
		{
			// Try other remaining zones with navigable points one by one, fail if none left
			CheckOnlyCurrentZone = false;
		}
		else return EActionStatus::Succeeded;
	}
	else if (!SO.IsStatic() || !ChildAction)
	{
		// Move to the closest zone with navigable point inside, fail if none left
		CheckOnlyCurrentZone = false;
	}
	else
	{
		return EActionStatus::Succeeded;
	}

	auto pSOSceneComponent = World.FindComponent<CSceneComponent>(pAction->_Object);
	if (!pSOSceneComponent || !pSOSceneComponent->RootNode) return EActionStatus::Failed;

	auto pActorSceneComponent = World.FindComponent<CSceneComponent>(EntityID);
	if (!pActorSceneComponent || !pActorSceneComponent->RootNode) return EActionStatus::Failed;

	const auto& ObjectWorldTfm = pSOSceneComponent->RootNode->GetWorldMatrix();
	matrix44 WorldToSmartObject;
	ObjectWorldTfm.invert_simple(WorldToSmartObject);

	const auto& ActorWorldTfm = pActorSceneComponent->RootNode->GetWorldMatrix();
	const auto& ActorPos = ActorWorldTfm.Translation();
	const vector3 SOSpaceActorPos = WorldToSmartObject.transform_coord(ActorPos);

	const auto* pNavAgent = World.FindComponent<AI::CNavAgentComponent>(EntityID);

	const auto PrevAllowedZones = pAction->_AllowedZones;

	while (CheckOnlyCurrentZone || pAction->_AllowedZones)
	{
		UPTR SegmentIdx = 0;
		float t = 0.f;
		if (CheckOnlyCurrentZone)
		{
			const CZone& Zone = SO.GetInteractionZone(pAction->_ZoneIndex).Zone;
			Zone.CalcSqDistance(SOSpaceActorPos, SegmentIdx, t);
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
				const CZone& Zone = SO.GetInteractionZone(i).Zone;
				const float SqDistance = Zone.CalcSqDistance(SOSpaceActorPos, CurrS, CurrT);
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

		vector3 ActionPos = ObjectWorldTfm.transform_coord(Zone.Zone.GetPoint(SOSpaceActorPos, SegmentIdx, t));

		const float WorldSqDistance = vector3::SqDistance2D(ActionPos, ActorPos);

		const float Radius = Zone.Zone.Radius; //???apply actor radius too?
		const float SqRadius = std::max(Radius * Radius, AI::Steer::SqLinearTolerance);

		// FIXME: what if there is something between ActionPos and ActorPos that blocks an interaction?
		// Use visibility raycast?
		if (WorldSqDistance <= SqRadius) return EActionStatus::Succeeded;

		ActionPos = vector3::lerp(ActionPos, ActorPos, Radius / n_sqrt(WorldSqDistance));

		vector3 FacingDir;
		float FacingTolerance;
		GetFacingParams(Zone, pAction->_Interaction, ObjectWorldTfm, ActionPos, FacingDir, FacingTolerance);

		// If character respects a navmesh and target is not in the same poly, must navigate to it
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
				if (CheckOnlyCurrentZone) break;
				continue;
			}
			else if (SqDiff > 0.f) ActionPos = NearestPos;

			dtPolyRef AgentPolyRef = pNavAgent->Corridor.getFirstPoly();
			if (pNavAgent->Mode == AI::ENavigationMode::Offmesh)
			{
				const float AgentExtents[3] = { pNavAgent->Radius, pNavAgent->Height, pNavAgent->Radius };
				pNavAgent->pNavQuery->findNearestPoly(ActorPos.v, AgentExtents, pNavAgent->Settings->GetQueryFilter(), &AgentPolyRef, NearestPos);
			}

			if (ObjPolyRef != AgentPolyRef)
			{
				// To avoid possible parent path invalidation, could try to optimize with findLocalNeighbourhood,
				// moveAlongSurface or raycast, but it would require additional logic and complicate navigation.
				Queue.PushOrUpdateChild<AI::Navigate>(Action, ActionPos, FacingDir, 0.f);
				return EActionStatus::Active;
			}
		}

		// For characters without navigation or in the same poly with target, no navigation required
		// TODO: if pNavAgent, get action from poly instead of hardcoded steering?
		Queue.PushOrUpdateChild<AI::Steer>(Action, ActionPos, ActionPos + FacingDir, 0.f);
		return EActionStatus::Active;
	}

	// No suitable zones left. Static SO fails, but dynamic still has a chance in subsequent frames.
	if (SO.IsStatic()) return EActionStatus::Failed;

	pAction->_AllowedZones = PrevAllowedZones;
	return EActionStatus::Active;
}
//---------------------------------------------------------------------

static EActionStatus FaceTarget(CGameWorld& World, HEntity EntityID, CActionQueueComponent& Queue, HAction Action,
	HAction ChildAction, EActionStatus ChildActionStatus, const CSmartObject& SO)
{
	if (auto pTurnAction = ChildAction.As<AI::Turn>())
	{
		if (ChildActionStatus == EActionStatus::Active)
		{
			// No need to update facing if the target is static
			if (SO.IsStatic()) return EActionStatus::Active;
		}
		else return ChildActionStatus; // May be only Failed or Succeeded here
	}

	auto pAction = Action.As<InteractWithSmartObject>();
	const auto& Zone = SO.GetInteractionZone(pAction->_ZoneIndex);

	auto pSOSceneComponent = World.FindComponent<CSceneComponent>(pAction->_Object);
	if (!pSOSceneComponent || !pSOSceneComponent->RootNode) return EActionStatus::Failed;

	auto pActorSceneComponent = World.FindComponent<CSceneComponent>(EntityID);
	if (!pActorSceneComponent || !pActorSceneComponent->RootNode) return EActionStatus::Failed;

	const auto& ObjectWorldTfm = pSOSceneComponent->RootNode->GetWorldMatrix();
	const auto& ActorWorldTfm = pActorSceneComponent->RootNode->GetWorldMatrix();

	vector3 TargetDir;
	float FacingTolerance;
	GetFacingParams(Zone, pAction->_Interaction, ObjectWorldTfm, ActorWorldTfm.Translation(), TargetDir, FacingTolerance);

	FacingTolerance = std::max(FacingTolerance, DEM::AI::Turn::AngularTolerance);

	vector3 LookatDir = -ActorWorldTfm.AxisZ();
	LookatDir.norm();
	const float Angle = vector3::Angle2DNorm(LookatDir, TargetDir);
	if (std::fabsf(Angle) < FacingTolerance) return EActionStatus::Succeeded;

	Queue.PushOrUpdateChild<AI::Turn>(Action, TargetDir, FacingTolerance);
	return EActionStatus::Active;
}
//---------------------------------------------------------------------

static EActionStatus InteractWithTarget(InteractWithSmartObject& Action, HEntity EntityID, AI::CAIStateComponent& AIState, const CSmartObject& SO, sol::state& Lua, float dt)
{
	// Start interaction, if not yet
	if (AIState.CurrInteractionTime < 0.f)
	{
		//!!!if animation graph override is defined, enable it!

		// TODO: wrap the call for safety! High risk of a typo in a SO script, game must be stable!
		if (auto LuaOnStart = SO.GetScriptFunction(Lua, "OnStart" + AIState.CurrInteraction.ToString()))
			LuaOnStart(EntityID, Action._Object);
		AIState.CurrInteractionTime = 0.f;
	}

	//!!!FIXME: Re-cache here in case an action is recreated! But in turn it leads to retrying
	// to cache absent OnUpdate every frame!
	if (!Action._UpdateScript)
		Action._UpdateScript = SO.GetScriptFunction(Lua, "OnUpdate" + AIState.CurrInteraction.ToString());

	//???first frame is 0.f or dt or unused part of dt?
	AIState.CurrInteractionTime += dt;

	if (Action._UpdateScript)
	{
		auto UpdateResult = Action._UpdateScript(EntityID, Action._Object, dt, AIState.CurrInteractionTime);
		if (!UpdateResult.valid())
		{
			sol::error Error = UpdateResult;
			::Sys::Error(Error.what());
			return EActionStatus::Failed;
		}
		else if (UpdateResult.get_type() == sol::type::number)
		{
			// Enums are represented as numbers in Sol
			EActionStatus NewStatus = UpdateResult;
			if (NewStatus != EActionStatus::Active) return NewStatus;
		}
		else
		{
			//!!!TODO: fmtlib and variadic args in assertion macros!
			n_assert2_dbg(UpdateResult.get_type() == sol::type::none, ("Unexpected return type from SO lua OnUpdate" + AIState.CurrInteraction.ToString()).c_str());
		}
	}

	return EActionStatus::Active;
}
//---------------------------------------------------------------------

static void EndCurrentInteraction(EActionStatus NewStatus, AI::CAIStateComponent& AIState, const CSmartObject* pSOAsset, HEntity EntityID, sol::state& Lua)
{
	if (!AIState.CurrInteraction) return;

	if (pSOAsset && AIState.CurrInteractionTime >= 0.f)
	{
		if (auto LuaOnEnd = pSOAsset->GetScriptFunction(Lua, "OnEnd" + AIState.CurrInteraction.ToString()))
		{
			// If we interrupt action by another NotQueued one, that simply means a cancellation of the previous action
			if (NewStatus == EActionStatus::NotQueued) NewStatus = EActionStatus::Cancelled;
			LuaOnEnd(EntityID, AIState.CurrTarget, NewStatus);
		}
	}

	AIState.CurrTarget = {};
	AIState.CurrInteraction = CStrID::Empty;
	AIState.CurrInteractionTime = -1.f;

	//!!!if animation graph override is enabled, disable it!
}
//---------------------------------------------------------------------

void UpdateAbilityInteractions(CGameWorld& World, sol::state& Lua, float dt)
{
	World.ForEachEntityWith<CActionQueueComponent, AI::CAIStateComponent>(
		[&World, &Lua, dt](auto EntityID, auto& Entity, CActionQueueComponent& Queue, AI::CAIStateComponent& AIState)
	{
		// Get the smart object currently being interacted with, if any
		auto pSOComponent = World.FindComponent<CSmartObjectComponent>(AIState.CurrTarget);
		auto pSOAsset = (pSOComponent && pSOComponent->Asset) ? pSOComponent->Asset->GetObject<CSmartObject>() : nullptr;

		// If current action is empty or has finished with any result, stop current interaction.
		// If child action was cancelled, the main action is considered cancelled too.
		const auto Action = Queue.FindCurrent<InteractWithSmartObject>();
		const auto ChildAction = Queue.GetChild(Action);
		const auto ChildActionStatus = Queue.GetStatus(ChildAction);
		const auto ActionStatus = (ChildActionStatus == EActionStatus::Cancelled) ? EActionStatus::Cancelled : Queue.GetStatus(Action);
		if (ActionStatus != EActionStatus::Active)
		{
			EndCurrentInteraction(ActionStatus, AIState, pSOAsset, EntityID, Lua);
			return;
		}

		auto pAction = Action.As<InteractWithSmartObject>();
		if (AIState.CurrTarget != pAction->_Object || AIState.CurrInteraction != pAction->_Interaction)
		{
			// Interrupt previous action, if it is active
			EndCurrentInteraction(EActionStatus::Cancelled, AIState, pSOAsset, EntityID, Lua);

			// Setup an actor AI from the action object
			AIState.CurrTarget = pAction->_Object;
			AIState.CurrInteraction = pAction->_Interaction;
			AIState.CurrInteractionTime = -1.f;
			pSOComponent = World.FindComponent<CSmartObjectComponent>(AIState.CurrTarget);
			pSOAsset = (pSOComponent && pSOComponent->Asset) ? pSOComponent->Asset->GetObject<CSmartObject>() : nullptr;
		}

		if (!pSOAsset)
		{
			EndCurrentInteraction(EActionStatus::Failed, AIState, pSOAsset, EntityID, Lua);
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
				EndCurrentInteraction(EActionStatus::Failed, AIState, pSOAsset, EntityID, Lua);
				Queue.SetStatus(Action, EActionStatus::Failed);
				return;
			}
		}

		auto Result = EActionStatus::Succeeded;

		// Move to and face interaction target. For non-static objects re-check during an interaction phase.
		if (!pSOAsset->IsStatic() || AIState.CurrInteractionTime < 0.f)
		{
			Result = MoveToTarget(World, EntityID, Queue, Action, ChildAction, ChildActionStatus, *pSOAsset);

			if (Result == EActionStatus::Succeeded)
				Result = FaceTarget(World, EntityID, Queue, Action, ChildAction, ChildActionStatus, *pSOAsset);

			if (Result == EActionStatus::Active && AIState.CurrInteractionTime >= 0.f)
			{
				// If needs moving or facing during an interaction phase, must interrupt an interaction
				EndCurrentInteraction(EActionStatus::Cancelled, AIState, pSOAsset, EntityID, Lua);
				AIState.CurrTarget = pAction->_Object;
				AIState.CurrInteraction = pAction->_Interaction;
				AIState.CurrInteractionTime = -1.f;
			}
		}

		// Interact with object, if it is approached and faced
		if (Result == EActionStatus::Succeeded)
			Result = InteractWithTarget(*pAction, EntityID, AIState, *pSOAsset, Lua, dt);

		// If all parts of interaction have finished, finish an action
		if (Result != EActionStatus::Active)
		{
			EndCurrentInteraction(Result, AIState, pSOAsset, EntityID, Lua);
			Queue.SetStatus(Action, Result);
		}
	});
}
//---------------------------------------------------------------------

}
