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

static bool GetFacingParams(const CAbilityInstance& AbilityInstance, const vector3& ActorPos, vector3& OutFacingDir, float* pOutFacingTolerance)
{
	CFacingParams Facing;
	if (AbilityInstance.Ability.GetFacingParams(Facing))
	{
		if (pOutFacingTolerance) *pOutFacingTolerance = std::max(Facing.Tolerance, AI::Turn::AngularTolerance);

		switch (Facing.Mode)
		{
			case EFacingMode::Direction:
			{
				OutFacingDir = Facing.Dir;
				return true;
			}
			case EFacingMode::Point:
			{
				OutFacingDir = AbilityInstance.TargetToWorld.transform_coord(Facing.Dir) - ActorPos;
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

 static void OptimizePath(CAbilityInstance& AbilityInstance, CGameWorld& World, HEntity EntityID, AI::Navigate& NavAction)
{
	if (AbilityInstance.PathOptimized) return;

	// Don't optimize until the final path is built
	// TODO: can also optimize partial path, another zone may intersect it. But need to reoptimize each time the path is updated!
	const auto* pNavAgent = World.FindComponent<AI::CNavAgentComponent>(EntityID);
	if (!pNavAgent || pNavAgent->State != AI::ENavigationState::Following || pNavAgent->Corridor.getLastPoly() != pNavAgent->TargetRef) return;

	AbilityInstance.PathOptimized = true;

	float Verts[DT_VERTS_PER_POLYGON * 3];
	const dtMeshTile* pTile = nullptr;
	const dtPoly* pPoly = nullptr;
	vector3 CornerInTargetSpace;
	bool Stop = false;

	// Find first poly that intersects with any of target zones
	const int PolyCount = pNavAgent->Corridor.getPathCount();
	for (int PolyIdx = 0; PolyIdx < PolyCount; ++PolyIdx)
	{
		const dtPolyRef PolyRef = pNavAgent->Corridor.getPath()[PolyIdx];
		pNavAgent->pNavQuery->getAttachedNavMesh()->getTileAndPolyByRefUnsafe(PolyRef, &pTile, &pPoly);

		const int PolyVertCount = pPoly->vertCount;
		for (int i = 0; i < PolyVertCount; ++i)
			dtVcopy(&Verts[i * 3], &pTile->verts[pPoly->verts[i] * 3]);

		float MinDistance = std::numeric_limits<float>().max();
		for (UPTR ZoneIdx = 0; ZoneIdx < AbilityInstance.AvailableZones.size(); ++ZoneIdx)
		{
			if (!AbilityInstance.AvailableZones[ZoneIdx]->IntersectsPoly(AbilityInstance.TargetToWorld, Verts, PolyVertCount)) continue;

			// Find previous path corner and stop on this poly
			if (!Stop)
			{
				vector3 Corner;
				vector3 NextCorner = pNavAgent->Corridor.getPos();
				dtPolyRef EnteredRef = pNavAgent->Corridor.getPath()[0];

				dtStraightPathContext Ctx;
				if (dtStatusFailed(pNavAgent->pNavQuery->initStraightPathSearch(
					Corner.v, pNavAgent->Corridor.getTarget(), pNavAgent->Corridor.getPath(), PolyCount, Ctx)))
				{
					return;
				}

				for (int PolyIdx2 = 0; PolyIdx2 <= PolyIdx; ++PolyIdx2)
				{
					if (EnteredRef != pNavAgent->Corridor.getPath()[PolyIdx2]) continue;
					Corner = NextCorner;
					if (PolyIdx2 == PolyIdx) break;
					if (!dtStatusInProgress(pNavAgent->pNavQuery->findNextStraightPathPoint(Ctx, NextCorner.v, nullptr, nullptr, &EnteredRef, 0)))
						return;
				}

				matrix44 WorldToTarget;
				AbilityInstance.TargetToWorld.invert_simple(WorldToTarget);
				CornerInTargetSpace = WorldToTarget.transform_coord(Corner);

				Stop = true;
			}

			// Find a zone intersecting this poly that is closest to the previous path corner
			//???adjust for actor radius? for now 0.f
			vector3 Point;
			const float Distance = AbilityInstance.AvailableZones[ZoneIdx]->FindClosestPoint(CornerInTargetSpace, 0.f, Point);
			if (Distance < MinDistance)
			{
				NavAction._Destination = Point; // Remember local for now, will convert once at the end
				AbilityInstance.CurrZoneIndex = ZoneIdx;
				MinDistance = Distance;
			}
		}

		if (Stop)
		{
			NavAction._Destination = AbilityInstance.TargetToWorld.transform_coord(NavAction._Destination);
			GetFacingParams(AbilityInstance, NavAction._Destination, NavAction._FinalFacing, nullptr);
			break;
		}
	}
}
//---------------------------------------------------------------------

static EActionStatus MoveToTarget(CAbilityInstance& AbilityInstance, CGameWorld& World, HEntity EntityID, CActionQueueComponent& Queue, HAction Action,
	HAction ChildAction, bool TransformChanged)
{
	//???FIXME: what if there were no zones initially? Allow to iact from actor's current position? Then must auto-succeed here!

	if (TransformChanged)
	{
		// Target transform changed, therefore all zones might have changed, must update action point
		AbilityInstance.PathOptimized = false;
		AbilityInstance.AvailableZones = AbilityInstance.InitialZones; // Intended copy
	}
	else if (ChildAction)
	{
		auto pNavAction = ChildAction.As<AI::Navigate>();
		if (!pNavAction && !ChildAction.As<AI::Steer>()) return EActionStatus::Succeeded; // Already at position
		const auto ChildActionStatus = Queue.GetStatus(ChildAction);
		if (ChildActionStatus != EActionStatus::Failed)
		{
			// If new path intersects with another interaction zone, can optimize by navigating to it instead of the original target
			if (pNavAction && ChildActionStatus == EActionStatus::Active)
				OptimizePath(AbilityInstance, World, EntityID, *pNavAction);
			return ChildActionStatus;
		}

		// Current zone can't be reached, try another one
		VectorFastErase(AbilityInstance.AvailableZones, AbilityInstance.CurrZoneIndex);
	}

	auto pActorSceneComponent = World.FindComponent<CSceneComponent>(EntityID);
	if (!pActorSceneComponent || !pActorSceneComponent->RootNode) return EActionStatus::Failed;

	matrix44 WorldToTarget;
	AbilityInstance.TargetToWorld.invert_simple(WorldToTarget);

	const auto& ActorWorldTfm = pActorSceneComponent->RootNode->GetWorldMatrix();
	const auto& ActorPos = ActorWorldTfm.Translation();
	const vector3 ActorPosInTargetSpace = WorldToTarget.transform_coord(ActorPos);

	const auto* pNavAgent = World.FindComponent<AI::CNavAgentComponent>(EntityID);

	while (!AbilityInstance.AvailableZones.empty())
	{
		// Find closest suitable zone and calculate action point in it
		// NB: could calculate once for each zone, sort and then loop over them, but most probably it will be slower than now,
		// because it is expected that almost always the first or at worst the second interaction zone will be selected.
		vector3 ActionPos;
		float MinDistance = std::numeric_limits<float>().max();
		for (UPTR i = 0; i < AbilityInstance.AvailableZones.size(); ++i)
		{
			//???adjust for actor radius? for now 0.f
			vector3 Point;
			const float Distance = AbilityInstance.AvailableZones[i]->FindClosestPoint(ActorPosInTargetSpace, 0.f, Point);
			if (Distance < MinDistance)
			{
				AbilityInstance.CurrZoneIndex = i;
				if (Distance <= AI::Steer::LinearTolerance) return EActionStatus::Succeeded;
				ActionPos = Point;
				MinDistance = Distance;
			}
		}

		// FIXME: what if something between ActionPos and ActorPos blocks an interaction? Use visibility raycast?
		ActionPos = AbilityInstance.TargetToWorld.transform_coord(ActionPos);

		// If actor uses navigation, find navigable point in a zone. Proceed to the next zone if failed.
		// NB: this check is simplified and may fail to navigate zones where reachable points exist, so, in order
		// to work, each zone must have at least one navigable point in a zone radius from each base point.
		// TODO: could instead explore all the interaction zone for intersecion with valid navigation polys, but it is slow.
		if (pNavAgent)
		{
			float NavigablePos[3];
			const float Radius = AbilityInstance.AvailableZones[AbilityInstance.CurrZoneIndex]->Radius;
			const float Extents[3] = { Radius, pNavAgent->Height, Radius };
			dtPolyRef ObjPolyRef = 0;
			pNavAgent->pNavQuery->findNearestPoly(ActionPos.v, Extents, pNavAgent->Settings->GetQueryFilter(), &ObjPolyRef, NavigablePos);

			if (!ObjPolyRef || dtVdist2DSqr(ActionPos.v, NavigablePos) > Radius * Radius)
			{
				VectorFastErase(AbilityInstance.AvailableZones, AbilityInstance.CurrZoneIndex);
				continue;
			}

			ActionPos = NavigablePos;
		}

		// Use target facing direction to improve arrival
		vector3 FacingDir;
		GetFacingParams(AbilityInstance, ActionPos, FacingDir, nullptr);

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

	// No zones left, fail
	// TODO: for dynamic targets may retry for some time before failing (target may change tfm in a second or two)
	//!!!can use ElapsedTime, if states move-face-prepare-execute will be explicit!
	return EActionStatus::Failed;
}
//---------------------------------------------------------------------

static EActionStatus FaceTarget(CAbilityInstance& AbilityInstance, CGameWorld& World, HEntity EntityID, CActionQueueComponent& Queue, HAction Action,
	HAction ChildAction, bool TransformChanged)
{
	// Process Turn sub-action until finished or target transform changed
	if (AbilityInstance.Status == EAbilityStatus::Facing && !TransformChanged) return Queue.GetStatus(ChildAction);

	auto pActorSceneComponent = World.FindComponent<CSceneComponent>(EntityID);
	if (!pActorSceneComponent || !pActorSceneComponent->RootNode) return EActionStatus::Failed;

	const auto& ActorWorldTfm = pActorSceneComponent->RootNode->GetWorldMatrix();

	vector3 TargetDir;
	float FacingTolerance;
	GetFacingParams(AbilityInstance, ActorWorldTfm.Translation(), TargetDir, &FacingTolerance);

	FacingTolerance = std::max(FacingTolerance, DEM::AI::Turn::AngularTolerance);

	vector3 LookatDir = -ActorWorldTfm.AxisZ();
	LookatDir.norm();
	const float Angle = vector3::Angle2DNorm(LookatDir, TargetDir);
	if (std::fabsf(Angle) < FacingTolerance) return EActionStatus::Succeeded;

	Queue.PushOrUpdateChild<AI::Turn>(Action, TargetDir, FacingTolerance);
	AbilityInstance.Status = EAbilityStatus::Facing;
	return EActionStatus::Active;
}
//---------------------------------------------------------------------

static EActionStatus InteractWithTarget(CAbilityInstance& AbilityInstance, HEntity EntityID, float dt)
{
	// Start interaction, if not yet
	if (AbilityInstance.Status != EAbilityStatus::Execution)
	{
		//!!!if animation graph override is defined, enable it!

		AbilityInstance.Ability.OnStart(); //???wrap into AbilityInstance.OnStart()? pass self as an argument inside!
		AbilityInstance.Status = EAbilityStatus::Execution;

		//???first frame must have elapsed time 0.f or dt or part of dt not used by movement and facing?
		AbilityInstance.PrevElapsedTime = 0.f;
		AbilityInstance.ElapsedTime = 0.f;
	}
	else
	{
		AbilityInstance.PrevElapsedTime = AbilityInstance.ElapsedTime;
		AbilityInstance.ElapsedTime += dt;
	}

	return AbilityInstance.Ability.OnUpdate(); //???wrap into AbilityInstance.OnUpdate()? pass self as an argument inside!

	//!!!move to scripted ability!
	/*
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

	return EActionStatus::Active;
	*/
}
//---------------------------------------------------------------------

static void EndCurrentInteraction(EActionStatus NewStatus, AI::CAIStateComponent& AIState, HEntity EntityID)
{
	// TODO: ability execution states instead of elapsed time checks?
	if (AIState._AbilityInstance->Status == EAbilityStatus::Execution)
	{
		if (NewStatus == EActionStatus::NotQueued) NewStatus = EActionStatus::Cancelled;
		AIState._AbilityInstance->Ability.OnEnd(); //???wrap into AbilityInstance.OnEnd()? pass self as an argument inside!
	}

	AIState._AbilityInstance = nullptr;

	//!!!if animation graph override is enabled, disable it!
}
//---------------------------------------------------------------------

void UpdateAbilityInteractions(CGameWorld& World, float dt)
{
	World.ForEachEntityWith<CActionQueueComponent, AI::CAIStateComponent>(
		[&World, dt](auto EntityID, auto& Entity, CActionQueueComponent& Queue, AI::CAIStateComponent& AIState)
	{
		// If current action is empty or has finished with any result, stop current interaction.
		// If child action was cancelled, the main action is considered cancelled too.
		const auto Action = Queue.FindCurrent<ExecuteAbility>();
		const auto ChildAction = Queue.GetChild(Action);
		const auto ActionStatus = (Queue.GetStatus(ChildAction) == EActionStatus::Cancelled) ? EActionStatus::Cancelled : Queue.GetStatus(Action);
		if (ActionStatus != EActionStatus::Active)
		{
			if (AIState._AbilityInstance) EndCurrentInteraction(ActionStatus, AIState, EntityID);
			return;
		}

		auto pAction = Action.As<ExecuteAbility>();
		if (!AIState._AbilityInstance && !pAction->_AbilityInstance)
		{
			// No ability is being executed
			Queue.SetStatus(Action, EActionStatus::Succeeded);
			return;
		}

		// ExecuteAbility action with empty ability instance means 'continue executing the current ability'.
		// This is due to unique_ptr used for ability instances. If made refcounted, can check equality here.
		const bool AbilityChanged = !!pAction->_AbilityInstance;
		if (AbilityChanged)
		{
			// NB: swap new ability with nullptr, because std::move doesn't guarantee donor validity after move
			if (AIState._AbilityInstance) EndCurrentInteraction(EActionStatus::Cancelled, AIState, EntityID);
			std::swap(AIState._AbilityInstance, pAction->_AbilityInstance);
		}

		CAbilityInstance& AbilityInstance = *AIState._AbilityInstance;

		// Determine target node, if any
		Scene::CSceneNode* pTargetRootNode = nullptr;
		bool TransformChanged = false;
		if (!AbilityInstance.Targets.empty())
		{
			const auto& Target = AbilityInstance.Targets[0];
			if (auto pTargetSceneComponent = World.FindComponent<CSceneComponent>(Target.Entity))
				pTargetRootNode = pTargetSceneComponent->RootNode;
			else
				pTargetRootNode = Target.pNode;
		}

		// Do one time initialization of the new ability instance
		if (AbilityChanged)
		{
			// Collect interaction zones
			//???Ability.GetZones beside SO zones, instead of them? what additional conditions may influence?
			//???should actor execute action without moving if no zones found at all?
			if (!AbilityInstance.Targets.empty())
			{
				auto pSOComponent = World.FindComponent<CSmartObjectComponent>(AbilityInstance.Targets[0].Entity);
				auto pSOAsset = (pSOComponent && pSOComponent->Asset) ? pSOComponent->Asset->GetObject<CSmartObject>() : nullptr;
				if (pSOAsset)
				{
					// TODO: can check additional conditions!
					const auto ZoneCount = pSOAsset->GetInteractionZoneCount();
					for (U8 i = 0; i < ZoneCount; ++i)
						AbilityInstance.InitialZones.push_back(&pSOAsset->GetInteractionZone(i).Zone);
				}
			}

			AbilityInstance.Ability.GetZones(AbilityInstance.InitialZones);

			TransformChanged = true;
			if (pTargetRootNode)
			{
				AbilityInstance.PrevTargetTfmVersion = pTargetRootNode->GetTransformVersion();
				AbilityInstance.TargetToWorld = pTargetRootNode->GetWorldMatrix();
			}
			else if (!AbilityInstance.Targets.empty())
			{
				// FIXME: AbilityInstance.TargetToWorld not needed if CTargetInfo will store full SRT instead of Point!
				AbilityInstance.TargetToWorld.Translation() = AbilityInstance.Targets[0].Point;
			}

			AIState._AbilityInstance->Status = EAbilityStatus::Movement;
		}
		else if (pTargetRootNode && AbilityInstance.PrevTargetTfmVersion != pTargetRootNode->GetTransformVersion())
		{
			TransformChanged = true;
			AbilityInstance.PrevTargetTfmVersion = pTargetRootNode->GetTransformVersion();
			AbilityInstance.TargetToWorld = pTargetRootNode->GetWorldMatrix();
		}

		// Move to and face interaction target

		auto Result = MoveToTarget(AbilityInstance, World, EntityID, Queue, Action, ChildAction, TransformChanged);

		if (Result == EActionStatus::Succeeded)
			Result = FaceTarget(AbilityInstance, World, EntityID, Queue, Action, ChildAction, TransformChanged);

		// TODO: ability execution states instead of elapsed time checks?
		if (Result == EActionStatus::Active && AbilityInstance.Status == EAbilityStatus::Execution)
		{
			// If needs moving or facing during an interaction phase, must interrupt an interaction
			//???TODO: add setting to interrupt or keep progress? Or progress is kept in target components?
			AbilityInstance.Ability.OnEnd(/*EActionStatus::Cancelled*/); //???wrap into AbilityInstance.OnEnd()? pass self as an argument inside!
			AbilityInstance.Status = EAbilityStatus::Movement;
		}

		// Interact with object, if it is approached and faced
		if (Result == EActionStatus::Succeeded)
			Result = InteractWithTarget(AbilityInstance, EntityID, dt);

		// If all parts of interaction have finished, finish an action
		if (Result != EActionStatus::Active)
		{
			EndCurrentInteraction(Result, AIState, EntityID);
			Queue.SetStatus(Action, Result);
		}
	});
}
//---------------------------------------------------------------------

}
