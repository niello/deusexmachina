#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <Scene/SceneComponent.h>
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

//!!!
// TODO: movement failed - add a timeout before failing, use ElapsedTime with Movement status
// TODO: what if ability requires visibility line from actor to target?
// TODO: anim graph override

namespace DEM::Game
{

static bool GetFacingParams(const CGameSession& Session, const CAbilityInstance& AbilityInstance, const rtm::vector4f& ActorPos, rtm::vector4f& OutFacingDir, float* pOutFacingTolerance)
{
	CFacingParams Facing;
	if (AbilityInstance.Ability.GetFacingParams(Session, AbilityInstance, Facing))
	{
		if (pOutFacingTolerance) *pOutFacingTolerance = std::max(n_deg2rad(Facing.Tolerance), AI::Turn::AngularTolerance);

		switch (Facing.Mode)
		{
			case EFacingMode::Direction:
			{
				OutFacingDir = Facing.Dir;
				return true;
			}
			case EFacingMode::Point:
			{
				OutFacingDir = rtm::vector_sub(rtm::matrix_mul_vector3(Facing.Dir, AbilityInstance.TargetToWorld), ActorPos);
				OutFacingDir = rtm::vector_normalize3(rtm::vector_set_y(OutFacingDir, 0.f));
				return true;
			}
		}
	}

	OutFacingDir = rtm::vector_zero();
	if (pOutFacingTolerance) *pOutFacingTolerance = AI::Turn::AngularTolerance;
	return false;
}
//---------------------------------------------------------------------

// When enter new navigation poly, check if it intersects an available zone. If so, move to it instead of original target zone.
static void OptimizePath(const CGameSession& Session, CAbilityInstance& AbilityInstance, CGameWorld& World, HEntity EntityID, HAction ChildAction)
{
	auto pNavAction = ChildAction.As<AI::Navigate>();
	if (!pNavAction) return;

	const auto* pNavAgent = World.FindComponent<AI::CNavAgentComponent>(EntityID);
	if (!pNavAgent || pNavAgent->Corridor.getFirstPoly() == AbilityInstance.CheckedPoly) return;

	AbilityInstance.CheckedPoly = pNavAgent->Corridor.getFirstPoly();
	if (!AbilityInstance.CheckedPoly) return;

	const rtm::matrix3x4f WorldToTarget = rtm::matrix_inverse(AbilityInstance.TargetToWorld);
	const rtm::vector4f ActorPosInTargetSpace = rtm::matrix_mul_point3(rtm::vector_load3(pNavAgent->Corridor.getPos()), WorldToTarget);

	const dtMeshTile* pTile = nullptr;
	const dtPoly* pPoly = nullptr;
	pNavAgent->pNavQuery->getAttachedNavMesh()->getTileAndPolyByRefUnsafe(AbilityInstance.CheckedPoly, &pTile, &pPoly);
	if (pPoly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION) return;

	rtm::vector4f Verts[DT_VERTS_PER_POLYGON];
	const int PolyVertCount = pPoly->vertCount;
	for (int i = 0; i < PolyVertCount; ++i)
		Verts[i] = rtm::vector_load3(&pTile->verts[pPoly->verts[i] * 3]);

	float MinDistance = std::numeric_limits<float>().max();
	for (UPTR ZoneIdx = 0; ZoneIdx < AbilityInstance.AvailableZones.size(); ++ZoneIdx)
	{
		const auto& Zone = *AbilityInstance.AvailableZones[ZoneIdx];

		// FIXME: need to find an union of zone and curent poly. Then must find a point in that union
		// closest to the actor position. Now the logic is bad and misses some opportunities.

		if (!Zone.IntersectsPoly(AbilityInstance.TargetToWorld, Verts, PolyVertCount)) continue;

		//???adjust for actor radius? for now 0.f
		rtm::vector4f Point;
		const float Distance = Zone.FindClosestPoint(ActorPosInTargetSpace, 0.f, Point);
		if (MinDistance <= Distance) continue;

		Point = rtm::matrix_mul_point3(Point, AbilityInstance.TargetToWorld);

		vector3 PointRaw = Math::FromSIMD3(Point);
		float NavigablePos[3];
		const float Extents[3] = { Zone.Radius, pNavAgent->Height, Zone.Radius };
		dtPolyRef ObjPolyRef = 0;
		pNavAgent->pNavQuery->findNearestPoly(PointRaw.v, Extents, pNavAgent->Settings->GetQueryFilter(), &ObjPolyRef, NavigablePos);
		if (ObjPolyRef != AbilityInstance.CheckedPoly || dtVdist2DSqr(PointRaw.v, NavigablePos) > Zone.Radius * Zone.Radius) continue;

		MinDistance = Distance;
		pNavAction->_Destination = Point; // Remember local for now, will convert once at the end
		AbilityInstance.CurrZoneIndex = ZoneIdx;
	}

	// If destination changed, update arrival facing
	if (MinDistance != std::numeric_limits<float>().max())
		GetFacingParams(Session, AbilityInstance, pNavAction->_Destination, pNavAction->_FinalFacing, nullptr);
}
//---------------------------------------------------------------------

static EActionStatus MoveToTarget(CGameSession& Session, CAbilityInstance& AbilityInstance, CGameWorld& World, HEntity EntityID,
	CActionQueueComponent& Queue, HAction Action, HAction ChildAction, bool TransformChanged)
{
	// When no zones available for an interaction, run ability from the current position
	//if (AbilityInstance.InitialZones.empty()) return EActionStatus::Succeeded;

	if (TransformChanged)
	{
		// Target transform changed, therefore all zones might have changed, must update action point
		AbilityInstance.AvailableZones = AbilityInstance.InitialZones; // Intended copy
		AbilityInstance.CheckedPoly = 0;
	}
	else if (AbilityInstance.Stage > EAbilityExecutionStage::Movement)
	{
		// Already at position, skip movement
		return EActionStatus::Succeeded;
	}
	else if (ChildAction)
	{
		// Process movement sub-action
		const auto ChildActionStatus = Queue.GetStatus(ChildAction);
		if (ChildActionStatus != EActionStatus::Failed)
		{
			// Check if another available zone is closer along the way than our target zone
			if (ChildActionStatus == EActionStatus::Active)
				OptimizePath(Session, AbilityInstance, World, EntityID, ChildAction);

			return ChildActionStatus;
		}

		// Movement failed. Current zone can't be reached, try another one.
		Algo::VectorFastErase(AbilityInstance.AvailableZones, AbilityInstance.CurrZoneIndex);
	}

	auto pActorSceneComponent = World.FindComponent<CSceneComponent>(EntityID);
	if (!pActorSceneComponent || !pActorSceneComponent->RootNode) return EActionStatus::Failed;

	const rtm::matrix3x4f WorldToTarget = rtm::matrix_inverse(AbilityInstance.TargetToWorld);
	const rtm::vector4f ActorPos = pActorSceneComponent->RootNode->GetWorldPosition();
	const rtm::vector4f ActorPosInTargetSpace = rtm::matrix_mul_point3(ActorPos, WorldToTarget);

	const auto* pNavAgent = World.FindComponent<AI::CNavAgentComponent>(EntityID);

	while (!AbilityInstance.AvailableZones.empty())
	{
		// Find closest suitable zone and calculate action point in it
		// NB: could calculate once for each zone, sort and then loop over them, but most probably it will be slower than now,
		// because it is expected that almost always the first or at worst the second interaction zone will be selected.
		rtm::vector4f ActionPos = rtm::vector_zero();
		float MinDistance = std::numeric_limits<float>().max();
		for (UPTR i = 0; i < AbilityInstance.AvailableZones.size(); ++i)
		{
			//???adjust for actor radius? for now 0.f
			rtm::vector4f Point;
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
		ActionPos = rtm::matrix_mul_point3(ActionPos, AbilityInstance.TargetToWorld);

		// If actor uses navigation, find navigable point in a zone. Proceed to the next zone if failed.
		// NB: this check is simplified and may fail to navigate zones where reachable points exist, so, in order
		// to work, each zone must have at least one navigable point in a zone radius from each base point.
		// TODO: could instead explore all the interaction zone for intersecion with valid navigation polys, but it is slow.
		// Can use Edelsbrunner's algorithm for distance between 2 convex polys. Distance < Zone.Radius = intersection.
		// http://acm.math.spbu.ru/~sk1/download/books/geometry/distance_O(log(n+m))_1985-J-02-ComputingExtremeDistances.pdf
		if (pNavAgent)
		{
			vector3 ActionPosRaw = Math::FromSIMD3(ActionPos);
			float NavigablePos[3];
			const float Radius = AbilityInstance.AvailableZones[AbilityInstance.CurrZoneIndex]->Radius;
			const float Extents[3] = { Radius, pNavAgent->Height, Radius };
			dtPolyRef ObjPolyRef = 0;
			pNavAgent->pNavQuery->findNearestPoly(ActionPosRaw.v, Extents, pNavAgent->Settings->GetQueryFilter(), &ObjPolyRef, NavigablePos);

			if (!ObjPolyRef || dtVdist2DSqr(ActionPosRaw.v, NavigablePos) > Radius * Radius)
			{
				Algo::VectorFastErase(AbilityInstance.AvailableZones, AbilityInstance.CurrZoneIndex);
				continue;
			}

			ActionPos = rtm::vector_load3(NavigablePos);
		}

		// Use target facing direction to improve arrival
		rtm::vector4f FacingDir;
		GetFacingParams(Session, AbilityInstance, ActionPos, FacingDir, nullptr);

		// If character is a navmesh agent, must navigate. Otherwise a simple steering does the job.
		// FIXME: Navigate action can't be nested now because it completely breaks offmesh traversal.
		// Steering may ignore special traversal logic but it is our only option at least for now.
		if (pNavAgent /*FIXME:*/ && !Queue.FindCurrent<AI::Navigate>(Action))
			Queue.PushOrUpdateChild<AI::Navigate>(Action, ActionPos, FacingDir, 0.f);
		else
			Queue.PushOrUpdateChild<AI::Steer>(Action, ActionPos, rtm::vector_add(ActionPos, FacingDir), 0.f);

		if (AbilityInstance.Stage == EAbilityExecutionStage::Interaction)
			AbilityInstance.Ability.OnEnd(Session, AbilityInstance, EActionStatus::Cancelled);
		AbilityInstance.Stage = EAbilityExecutionStage::Movement;

		return EActionStatus::Active;
	}

	// No zones left, fail
	// TODO: for dynamic targets may retry for some time before failing (target may change tfm in a second or two)
	//!!!can use ElapsedTime, if states move-face-prepare-execute will be explicit!
	return EActionStatus::Failed;
}
//---------------------------------------------------------------------

static EActionStatus FaceTarget(CGameSession& Session, CAbilityInstance& AbilityInstance, CGameWorld& World, HEntity EntityID,
	CActionQueueComponent& Queue, HAction Action, HAction ChildAction, bool TransformChanged)
{
	// Process Turn sub-action until finished or target transform changed
	if (!TransformChanged)
	{
		if (AbilityInstance.Stage > EAbilityExecutionStage::Facing) return EActionStatus::Succeeded;
		else if (ChildAction.As<AI::Turn>()) return Queue.GetStatus(ChildAction);
	}

	auto pActorSceneComponent = World.FindComponent<CSceneComponent>(EntityID);
	if (!pActorSceneComponent || !pActorSceneComponent->RootNode) return EActionStatus::Failed;

	const auto& ActorWorldTfm = pActorSceneComponent->RootNode->GetWorldMatrix();

	rtm::vector4f TargetDir;
	float FacingTolerance;
	GetFacingParams(Session, AbilityInstance, ActorWorldTfm.w_axis, TargetDir, &FacingTolerance);

	FacingTolerance = std::max(FacingTolerance, DEM::AI::Turn::AngularTolerance);

	const rtm::vector4f LookatDir = rtm::vector_normalize3(rtm::vector_neg(ActorWorldTfm.z_axis));
	const float Angle = Math::AngleXZNorm(LookatDir, TargetDir);
	if (std::fabsf(Angle) < FacingTolerance) return EActionStatus::Succeeded;

	Queue.PushOrUpdateChild<AI::Turn>(Action, TargetDir, FacingTolerance);

	if (AbilityInstance.Stage == EAbilityExecutionStage::Interaction)
		AbilityInstance.Ability.OnEnd(Session, AbilityInstance, EActionStatus::Cancelled);
	AbilityInstance.Stage = EAbilityExecutionStage::Facing;

	return EActionStatus::Active;
}
//---------------------------------------------------------------------

static EActionStatus InteractWithTarget(CGameSession& Session, CAbilityInstance& AbilityInstance, HEntity EntityID, float dt)
{
	// Start interaction, if not yet
	if (AbilityInstance.Stage != EAbilityExecutionStage::Interaction)
	{
		//!!!if animation graph override is defined, enable it!

		AbilityInstance.Ability.OnStart(Session, AbilityInstance);
		AbilityInstance.Stage = EAbilityExecutionStage::Interaction;

		//???first frame must have elapsed time 0.f or dt or part of dt not used by movement and facing?
		AbilityInstance.PrevElapsedTime = 0.f;
		AbilityInstance.ElapsedTime = 0.f;
	}
	else
	{
		AbilityInstance.PrevElapsedTime = AbilityInstance.ElapsedTime;
		AbilityInstance.ElapsedTime += dt;
	}

	return AbilityInstance.Ability.OnUpdate(Session, AbilityInstance);
}
//---------------------------------------------------------------------

static void EndCurrentInteraction(CGameSession& Session, EActionStatus NewStatus, AI::CAIStateComponent& AIState, HEntity EntityID)
{
	if (AIState._AbilityInstance->Stage == EAbilityExecutionStage::Interaction)
	{
		if (NewStatus == EActionStatus::NotQueued) NewStatus = EActionStatus::Cancelled;
		AIState._AbilityInstance->Ability.OnEnd(Session, *AIState._AbilityInstance, NewStatus);
	}

	AIState._AbilityInstance = nullptr;

	//!!!if animation graph override is enabled, disable it!
}
//---------------------------------------------------------------------

void UpdateAbilityInteractions(CGameSession& Session, CGameWorld& World, float dt)
{
	World.ForEachEntityWith<CActionQueueComponent, AI::CAIStateComponent>(
		[&Session, &World, dt](auto EntityID, auto& Entity, CActionQueueComponent& Queue, AI::CAIStateComponent& AIState)
	{
		// If current action is empty or has finished with any result, stop current interaction.
		// If child action was cancelled, the main action is considered cancelled too.
		const auto Action = Queue.FindCurrent<ExecuteAbility>();
		const auto ChildAction = Queue.GetChild(Action);
		const auto ActionStatus = (Queue.GetStatus(ChildAction) == EActionStatus::Cancelled) ? EActionStatus::Cancelled : Queue.GetStatus(Action);
		if (ActionStatus != EActionStatus::Active)
		{
			if (AIState._AbilityInstance) EndCurrentInteraction(Session, ActionStatus, AIState, EntityID);
			return;
		}

		auto pAction = Action.As<ExecuteAbility>();
		const bool AbilityChanged = (pAction->_AbilityInstance != AIState._AbilityInstance);
		if (AbilityChanged)
		{
			// Interrupt previous ability
			// NB: it might be our parent, then it will be resumed when we finish executing the nested one
			if (AIState._AbilityInstance) EndCurrentInteraction(Session, EActionStatus::Cancelled, AIState, EntityID);

			AIState._AbilityInstance = pAction->_AbilityInstance;

			// Initialize new ability
			if (AIState._AbilityInstance)
			{
				AIState._AbilityInstance->Actor = EntityID;
				AIState._AbilityInstance->Stage = EAbilityExecutionStage::Movement;

				// Get interaction zones from a smart object, if present
				if (!AIState._AbilityInstance->Targets.empty())
				{
					auto pSOComponent = World.FindComponent<const CSmartObjectComponent>(AIState._AbilityInstance->Targets[0].Entity);
					auto pSOAsset = (pSOComponent && pSOComponent->Asset) ? pSOComponent->Asset->GetObject<CSmartObject>() : nullptr;
					if (pSOAsset)
					{
						// TODO: can check additional conditions of zone before adding it!
						const auto ZoneCount = pSOAsset->GetInteractionZoneCount();
						AIState._AbilityInstance->InitialZones.reserve(ZoneCount);
						for (U8 i = 0; i < ZoneCount; ++i)
							AIState._AbilityInstance->InitialZones.push_back(&pSOAsset->GetInteractionZone(i));
					}
				}

				// Get interaction zones from an ability
				AIState._AbilityInstance->Ability.GetZones(Session, *AIState._AbilityInstance, AIState._AbilityInstance->InitialZones);
				n_assert_dbg(!AIState._AbilityInstance->InitialZones.empty());
			}
		}

		if (!AIState._AbilityInstance)
		{
			// No ability is being executed
			Queue.SetStatus(Action, EActionStatus::Succeeded);
			return;
		}

		CAbilityInstance& AbilityInstance = *AIState._AbilityInstance;

		// Determine target node, if any
		Scene::CSceneNode* pTargetRootNode = nullptr;
		if (!AbilityInstance.Targets.empty())
		{
			const auto& Target = AbilityInstance.Targets[0];
			if (auto pTargetSceneComponent = World.FindComponent<CSceneComponent>(Target.Entity))
				pTargetRootNode = pTargetSceneComponent->RootNode;
			else
				pTargetRootNode = Target.pNode;
		}

		// Update main target transform
		bool TransformChanged = false;
		if (AbilityChanged)
		{
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
		}
		else if (pTargetRootNode && AbilityInstance.PrevTargetTfmVersion != pTargetRootNode->GetTransformVersion())
		{
			TransformChanged = true;
			AbilityInstance.PrevTargetTfmVersion = pTargetRootNode->GetTransformVersion();
			AbilityInstance.TargetToWorld = pTargetRootNode->GetWorldMatrix();
		}

		// Ability execution logic

		EActionStatus Result = MoveToTarget(Session, AbilityInstance, World, EntityID, Queue, Action, ChildAction, TransformChanged);

		if (Result == EActionStatus::Succeeded)
			Result = FaceTarget(Session, AbilityInstance, World, EntityID, Queue, Action, ChildAction, TransformChanged);

		if (Result == EActionStatus::Succeeded)
			Result = InteractWithTarget(Session, AbilityInstance, EntityID, dt);

		if (Result != EActionStatus::Active)
		{
			EndCurrentInteraction(Session, Result, AIState, EntityID);
			Queue.SetStatus(Action, Result);
		}
	});
}
//---------------------------------------------------------------------

}
