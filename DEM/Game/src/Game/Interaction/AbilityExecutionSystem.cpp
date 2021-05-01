#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <Game/ECS/Components/SceneComponent.h>
#include <AI/Navigation/NavAgentComponent.h>
#include <AI/AIStateComponent.h>
#include <AI/Movement/SteerAction.h>
#include <Game/Interaction/Ability.h>
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

// FIXME: is optimized point used somewhere? Seems like we optimize but continue moving to the old point!
// FIXME: distance from segment to poly and from poly to poly are not calculated yet, so optimization works only
// for point/circle zones. Maybe a cheaper way is to update sub-action when enter new poly or by timer?
static void OptimizePath(CAbilityInstance& AbilityInstance, CGameWorld& World, HEntity EntityID, AI::Navigate& NavAction, const CSmartObject& SO)
{
	//!!!TODO: optimize only if the target poly is the end poly, no partial path optimization should happen!
	//!!!TODO: early exit if no other zones exist! But may also optimize in its own zone and check the last poly too!
	//!!!TODO: calc new point for navigation!
	//!!!TODO: instead of PathOptimized could increment path version on replan and recheck,
	//to handle partial path and corridor optimizations!
	// Don't optimize path to dynamic target, effort will be invalidated after the next target move or rotation
	if (!SO.IsStatic() || AbilityInstance.PathOptimized) return;

	auto pSOSceneComponent = World.FindComponent<CSceneComponent>(AbilityInstance.Targets[0].Entity);
	if (!pSOSceneComponent || !pSOSceneComponent->RootNode) return;
	const auto& ObjectWorldTfm = pSOSceneComponent->RootNode->GetWorldMatrix();

	const auto* pNavAgent = World.FindComponent<AI::CNavAgentComponent>(EntityID);
	if (!pNavAgent || pNavAgent->State != AI::ENavigationState::Following) return;

	AbilityInstance.PathOptimized = true;

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
			if (!((1 << ZoneIdx) & AbilityInstance._AllowedZones)) continue;
			const CZone& Zone = SO.GetInteractionZone(ZoneIdx).Zone;
			if (Zone.IntersectsNavPoly(ObjectWorldTfm, Verts, PolyVertCount, NavAction._Destination))
			{
				AbilityInstance._ZoneIndex = ZoneIdx;
				return;
			}
		}
	}
}
//---------------------------------------------------------------------

static bool GetFacingParams(const CInteractionZone& Zone, CStrID InteractionID, const matrix44& ObjectWorldTfm,
	const vector3& ActorPos, vector3& OutFacingDir, float* pOutFacingTolerance)
{
	auto It = std::find_if(Zone.Interactions.cbegin(), Zone.Interactions.cend(), [InteractionID](const auto& Elm)
	{
		return Elm.ID == InteractionID;
	});

	if (It != Zone.Interactions.cend()) // Should always be true
	{
		if (pOutFacingTolerance) *pOutFacingTolerance = It->FacingTolerance;

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
	if (pOutFacingTolerance) *pOutFacingTolerance = AI::Turn::AngularTolerance;
	return false;
}
//---------------------------------------------------------------------

//!!!Check if target tfm changed, reset all zones, update target point, remember new tfm.
//fail if no zones left, but they were initially (InitialZones not empty)
static EActionStatus MoveToTarget(CAbilityInstance& AbilityInstance, CGameWorld& World, HEntity EntityID, CActionQueueComponent& Queue, HAction Action,
	HAction ChildAction, EActionStatus ChildActionStatus, const CSmartObject& SO)
{
	n_assert(!AbilityInstance.Targets.empty()); //???can happen? return Succeeded in this case?

	auto pTargetSceneComponent = World.FindComponent<CSceneComponent>(AbilityInstance.Targets[0].Entity);
	if (!pTargetSceneComponent || !pTargetSceneComponent->RootNode) return EActionStatus::Failed;

	auto pActorSceneComponent = World.FindComponent<CSceneComponent>(EntityID);
	if (!pActorSceneComponent || !pActorSceneComponent->RootNode) return EActionStatus::Failed;

	const U32 TargetTfmVersion = pTargetSceneComponent->RootNode->GetTransformVersion();
	if (AbilityInstance.PrevTargetTfmVersion != TargetTfmVersion)
	{
		AbilityInstance.PrevTargetTfmVersion = TargetTfmVersion;
		AbilityInstance.PathOptimized = false;
		AbilityInstance.AvailableZones = AbilityInstance.InitialZones;
	}
	else if (ChildAction)
	{
		auto pSteerAction = ChildAction.As<AI::Steer>();
		auto pNavAction = ChildAction.As<AI::Navigate>();
		if (!pSteerAction && !pNavAction) return EActionStatus::Succeeded; // Already at position
		if (ChildActionStatus != EActionStatus::Failed)
		{
			// If new path intersects with another interaction zone, can optimize by navigating to it instead of the original target
			if (ChildActionStatus == EActionStatus::Active)
				OptimizePath(AbilityInstance, World, EntityID, *pNavAction, SO);
			return ChildActionStatus;
		}
	}

	// update point, and if it changed, interrupt possible interaction, add movement sub-action & reset path optimized flag
	// if no zones left, fail (for dynamic targets may retry for some time before failing)

	auto pAction = Action.As<ExecuteAbility>();

	const auto& TargetToWorld = pTargetSceneComponent->RootNode->GetWorldMatrix();
	matrix44 WorldToTarget;
	TargetToWorld.invert_simple(WorldToTarget);

	const auto& ActorWorldTfm = pActorSceneComponent->RootNode->GetWorldMatrix();
	const auto& ActorPos = ActorWorldTfm.Translation();
	const vector3 ActorPosInTargetSpace = WorldToTarget.transform_coord(ActorPos);

	const auto* pNavAgent = World.FindComponent<AI::CNavAgentComponent>(EntityID);

	while (pAction->_AbilityInstance->_AllowedZones)
	{
		// Find closest suitable zone and calculate action point prerequisites
		// NB: could calculate once, sort and then loop over them, but most probably it will be slower than now,
		// because it is expected that almost always the first or at worst the second interaction zone will be selected.
		UPTR SegmentIdx = 0;
		float t = 0.f;
		float MinSqDistance = std::numeric_limits<float>().max();
		const auto ZoneCount = SO.GetInteractionZoneCount();
		for (U8 i = 0; i < ZoneCount; ++i)
		{
			if (!((1 << i) & pAction->_AbilityInstance->_AllowedZones)) continue;

			UPTR CurrS;
			float CurrT;
			const CZone& Zone = SO.GetInteractionZone(i).Zone;
			const float SqDistance = Zone.CalcSqDistance(ActorPosInTargetSpace, CurrS, CurrT);
			if (SqDistance < MinSqDistance)
			{
				MinSqDistance = SqDistance;
				SegmentIdx = CurrS;
				t = CurrT;
				pAction->_AbilityInstance->_ZoneIndex = i;
			}
		}

		// Calculate action point 

		const auto& Zone = SO.GetInteractionZone(pAction->_AbilityInstance->_ZoneIndex);

		const float Radius = Zone.Zone.Radius; //???apply actor radius too?
		const float SqRadius = std::max(Radius * Radius, AI::Steer::SqLinearTolerance);

		vector3 ActionPos = TargetToWorld.transform_coord(Zone.Zone.GetPoint(ActorPosInTargetSpace, SegmentIdx, t));

		// FIXME: what if there is something between ActionPos and ActorPos that blocks an interaction?
		// Use visibility raycast?
		const float WorldSqDistance = vector3::SqDistance2D(ActionPos, ActorPos);
		if (WorldSqDistance <= SqRadius) return EActionStatus::Succeeded;

		ActionPos = vector3::lerp(ActionPos, ActorPos, Radius / n_sqrt(WorldSqDistance));

		// If actor uses navigation, find navigable point in a zone. Proceed to the next zone if failed.
		if (pNavAgent)
		{
			float NavigablePos[3];
			const float Extents[3] = { Radius, pNavAgent->Height, Radius };
			dtPolyRef ObjPolyRef = 0;
			pNavAgent->pNavQuery->findNearestPoly(ActionPos.v, Extents, pNavAgent->Settings->GetQueryFilter(), &ObjPolyRef, NavigablePos);

			// NB: this check is simplified and may fail to navigate zones where reachable points exist, so, in order
			// to work, each zone must have at least one navigable point in a zone radius from each base point.
			// TODO: could instead explore all the interaction zone for intersecion with valid navigation polys, but it is slow.
			if (!ObjPolyRef || dtVdist2DSqr(ActionPos.v, NavigablePos) > SqRadius)
			{
				pAction->_AbilityInstance->_AllowedZones &= ~(1 << pAction->_AbilityInstance->_ZoneIndex);
				continue;
			}

			ActionPos = NavigablePos;
		}

		// Use target facing direction to improve arrival
		vector3 FacingDir;
		GetFacingParams(Zone, pAction->_AbilityInstance->_Interaction, TargetToWorld, ActionPos, FacingDir, nullptr);

		// If character is a navmesh agent, must navigate. Otherwise a simple steering does the job.
		// NB: navigate even if already at the target poly, because it may require special traversal action, not Steer.
		// To avoid possible parent path invalidation, could try to optimize with findLocalNeighbourhood,
		// moveAlongSurface or raycast, but it would require additional logic and complicate navigation.
		if (pNavAgent)
			Queue.PushOrUpdateChild<AI::Navigate>(Action, ActionPos, FacingDir, 0.f);
		else
			Queue.PushOrUpdateChild<AI::Steer>(Action, ActionPos, ActionPos + FacingDir, 0.f);

		return EActionStatus::Active;
	}

	// No suitable zones left. Static SO fails, but dynamic still has a chance in subsequent frames.
	if (SO.IsStatic()) return EActionStatus::Failed;

	pAction->_AbilityInstance->_AllowedZones = PrevAllowedZones;
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

	auto pAction = Action.As<ExecuteAbility>();
	const auto& Zone = SO.GetInteractionZone(pAction->_AbilityInstance->_ZoneIndex);

	auto pSOSceneComponent = World.FindComponent<CSceneComponent>(pAction->_AbilityInstance->_Object);
	if (!pSOSceneComponent || !pSOSceneComponent->RootNode) return EActionStatus::Failed;

	auto pActorSceneComponent = World.FindComponent<CSceneComponent>(EntityID);
	if (!pActorSceneComponent || !pActorSceneComponent->RootNode) return EActionStatus::Failed;

	const auto& ObjectWorldTfm = pSOSceneComponent->RootNode->GetWorldMatrix();
	const auto& ActorWorldTfm = pActorSceneComponent->RootNode->GetWorldMatrix();

	vector3 TargetDir;
	float FacingTolerance;
	GetFacingParams(Zone, pAction->_AbilityInstance->_Interaction, ObjectWorldTfm, ActorWorldTfm.Translation(), TargetDir, &FacingTolerance);

	FacingTolerance = std::max(FacingTolerance, DEM::AI::Turn::AngularTolerance);

	vector3 LookatDir = -ActorWorldTfm.AxisZ();
	LookatDir.norm();
	const float Angle = vector3::Angle2DNorm(LookatDir, TargetDir);
	if (std::fabsf(Angle) < FacingTolerance) return EActionStatus::Succeeded;

	Queue.PushOrUpdateChild<AI::Turn>(Action, TargetDir, FacingTolerance);
	return EActionStatus::Active;
}
//---------------------------------------------------------------------

static EActionStatus InteractWithTarget(ExecuteAbility& Action, HEntity EntityID, AI::CAIStateComponent& AIState, const CSmartObject& SO, sol::state& Lua, float dt)
{
	// Start interaction, if not yet
	if (AIState.CurrInteractionTime < 0.f)
	{
		//!!!if animation graph override is defined, enable it!

		// TODO: wrap the call for safety! High risk of a typo in a SO script, game must be stable!
		if (auto LuaOnStart = SO.GetScriptFunction(Lua, "OnStart" + AIState.CurrInteraction.ToString()))
			LuaOnStart(EntityID, Action._AbilityInstance->_Object);
		AIState.CurrInteractionTime = 0.f;
	}

	//!!!FIXME: Re-cache here in case an action is recreated! But in turn it leads to retrying
	// to cache absent OnUpdate every frame!
	if (!Action._AbilityInstance->_UpdateScript)
		Action._AbilityInstance->_UpdateScript = SO.GetScriptFunction(Lua, "OnUpdate" + AIState.CurrInteraction.ToString());

	//???first frame is 0.f or dt or unused part of dt?
	AIState.CurrInteractionTime += dt;

	if (Action._AbilityInstance->_UpdateScript)
	{
		auto UpdateResult = Action._AbilityInstance->_UpdateScript(EntityID, Action._AbilityInstance->_Object, dt, AIState.CurrInteractionTime);
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
		const auto Action = Queue.FindCurrent<ExecuteAbility>();
		const auto ChildAction = Queue.GetChild(Action);
		const auto ChildActionStatus = Queue.GetStatus(ChildAction);
		const auto ActionStatus = (ChildActionStatus == EActionStatus::Cancelled) ? EActionStatus::Cancelled : Queue.GetStatus(Action);
		if (ActionStatus != EActionStatus::Active)
		{
			EndCurrentInteraction(ActionStatus, AIState, pSOAsset, EntityID, Lua);
			return;
		}

		// ExecuteAbility action with empty ability instance means 'continue executing the current ability'.
		// This is due to unique_ptr used for ability instances. If made refcounted, can check equality here.
		auto pAction = Action.As<ExecuteAbility>();
		if (pAction->_AbilityInstance)
		{
			// Interrupt previous action, if it is active
			EndCurrentInteraction(EActionStatus::Cancelled, AIState, pSOAsset, EntityID, Lua);

			// Start new ability instance execution
			AIState._AbilityInstance = std::move(Action._AbilityInstance);
			pSOComponent = World.FindComponent<CSmartObjectComponent>(AIState.CurrTarget);
			pSOAsset = (pSOComponent && pSOComponent->Asset) ? pSOComponent->Asset->GetObject<CSmartObject>() : nullptr;

			if (pSOAsset)
			{
				// TODO: can check additional conditions!
				const auto ZoneCount = pSOAsset->GetInteractionZoneCount();
				for (U8 i = 0; i < ZoneCount; ++i)
					AIState._AbilityInstance->InitialZones.push_back(&pSOAsset->GetInteractionZone(i));
			}

			//???beside SO zones, instead of them? what additional conditions may influence?
			AIState._AbilityInstance->Ability.GetZones(AIState._AbilityInstance->InitialZones);

			//!!!must update available zones and reset transform. May be done auto below, if detects difference. Zero out tfm?
			//!!!???fail if no zones - auto below!?
			//???should execute action in a current place if no zones returned at all?
		}
		else if (!AIState._AbilityInstance)
		{
			// No ability is being executed
			Queue.SetStatus(Action, EActionStatus::Succeeded);
			return;
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
				AIState.CurrTarget = pAction->_AbilityInstance->_Object;
				AIState.CurrInteraction = pAction->_AbilityInstance->_Interaction;
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
