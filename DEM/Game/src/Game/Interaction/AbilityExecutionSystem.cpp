#include <Game/ECS/GameWorld.h>
#include <Scene/SceneComponent.h>
#include <AI/Navigation/NavAgentComponent.h>
#include <AI/AIStateComponent.h>
#include <AI/CommandStackComponent.h>
#include <AI/Movement/SteerAction.h>
#include <Game/Interaction/Ability.h>
#include <Game/Interaction/AbilityInstance.h>
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
				OutFacingDir = Math::ToSIMD(Facing.Dir);
				return true;
			}
			case EFacingMode::Point:
			{
				OutFacingDir = rtm::vector_sub(rtm::matrix_mul_point3(Math::ToSIMD(Facing.Dir), AbilityInstance.TargetToWorld), ActorPos);
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
static void OptimizePath(const CGameSession& Session, CAbilityInstance& AbilityInstance, CGameWorld& World, HEntity EntityID, AI::CCommandFuture& SubCmd)
{
	auto* pNavAction = SubCmd.As<AI::Navigate>();
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

static AI::ECommandStatus MoveToTarget(CGameSession& Session, CAbilityInstance& AbilityInstance, CGameWorld& World, HEntity EntityID,
	AI::CCommandStackComponent& CmdStack, ExecuteAbility& Cmd, bool TransformChanged)
{
	// When no zones available for an interaction, run ability from the current position
	//if (AbilityInstance.InitialZones.empty()) return AI::ECommandStatus::Succeeded;

	if (TransformChanged)
	{
		// Target transform changed, therefore all zones might have changed, must update action point
		AbilityInstance.AvailableZones = AbilityInstance.InitialZones; // Intended copy
		AbilityInstance.CheckedPoly = 0;
	}
	else if (AbilityInstance.Stage > EAbilityExecutionStage::Movement)
	{
		// Already at position, skip movement
		return AI::ECommandStatus::Succeeded;
	}
	else if (Cmd._SubCommandFuture)
	{
		// Process movement sub-command. Stage == Movement guarantees that this is movement,
		// and check inside CalcActualAbilityStatus guarantees that it is not cancelled.
		const auto SubCmdStatus = Cmd._SubCommandFuture.GetStatus();
		if (SubCmdStatus != AI::ECommandStatus::Failed)
		{
			if (SubCmdStatus == AI::ECommandStatus::Succeeded)
				Cmd._SubCommandFuture = {};
			else
				// Check if another available zone is closer along the way than our target zone
				OptimizePath(Session, AbilityInstance, World, EntityID, Cmd._SubCommandFuture);

			// Continue running or succeed
			return SubCmdStatus;
		}

		// Movement failed. Current zone can't be reached, try another one.
		Algo::VectorFastErase(AbilityInstance.AvailableZones, AbilityInstance.CurrZoneIndex);
		Cmd._SubCommandFuture = {};
	}

	auto* pActorSceneComponent = World.FindComponent<const CSceneComponent>(EntityID);
	if (!pActorSceneComponent || !pActorSceneComponent->RootNode) return AI::ECommandStatus::Failed;

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
				if (Distance <= AI::Steer::LinearTolerance) return AI::ECommandStatus::Succeeded;
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
		if (pNavAgent /*FIXME:*/ && !CmdStack.FindTopmostCommand<AI::Navigate>())
			AI::PushOrUpdateCommand<AI::Navigate>(CmdStack, Cmd._SubCommandFuture, ActionPos, FacingDir, 0.f);
		else
			AI::PushOrUpdateCommand<AI::Steer>(CmdStack, Cmd._SubCommandFuture, ActionPos, rtm::vector_add(ActionPos, FacingDir), 0.f);

		if (AbilityInstance.Stage == EAbilityExecutionStage::Interaction)
			AbilityInstance.Ability.OnEnd(Session, AbilityInstance, AI::ECommandStatus::Cancelled);
		AbilityInstance.Stage = EAbilityExecutionStage::Movement;

		return AI::ECommandStatus::Running;
	}

	// No zones left, fail
	// TODO: for dynamic targets may retry for some time before failing (target may change tfm in a second or two)
	//!!!can use ElapsedTime, if states move-face-prepare-execute will be explicit!
	return AI::ECommandStatus::Failed;
}
//---------------------------------------------------------------------

static AI::ECommandStatus FaceTarget(CGameSession& Session, CAbilityInstance& AbilityInstance, CGameWorld& World, HEntity EntityID,
	AI::CCommandStackComponent& CmdStack, ExecuteAbility& Cmd, bool TransformChanged)
{
	// Process Turn sub-command until finished or target transform changed
	if (!TransformChanged)
	{
		if (AbilityInstance.Stage > EAbilityExecutionStage::Facing)
			return AI::ECommandStatus::Succeeded;

		if (Cmd._SubCommandFuture.As<AI::Turn>())
		{
			const auto SubCmdStatus = Cmd._SubCommandFuture.GetStatus();
			if (AI::IsTerminalCommandStatus(SubCmdStatus))
				Cmd._SubCommandFuture = {};
			return SubCmdStatus;
		}
	}

	auto* pActorSceneComponent = World.FindComponent<const CSceneComponent>(EntityID);
	if (!pActorSceneComponent || !pActorSceneComponent->RootNode) return AI::ECommandStatus::Failed;

	const auto& ActorWorldTfm = pActorSceneComponent->RootNode->GetWorldMatrix();

	rtm::vector4f TargetDir;
	float FacingTolerance;
	GetFacingParams(Session, AbilityInstance, ActorWorldTfm.w_axis, TargetDir, &FacingTolerance);

	FacingTolerance = std::max(FacingTolerance, DEM::AI::Turn::AngularTolerance);

	const rtm::vector4f LookatDir = rtm::vector_normalize3(rtm::vector_neg(ActorWorldTfm.z_axis));
	const float Angle = Math::AngleXZNorm(LookatDir, TargetDir);
	if (std::fabsf(Angle) < FacingTolerance) return AI::ECommandStatus::Succeeded;

	AI::PushOrUpdateCommand<AI::Turn>(CmdStack, Cmd._SubCommandFuture, TargetDir, FacingTolerance);

	if (AbilityInstance.Stage == EAbilityExecutionStage::Interaction)
		AbilityInstance.Ability.OnEnd(Session, AbilityInstance, AI::ECommandStatus::Cancelled);
	AbilityInstance.Stage = EAbilityExecutionStage::Facing;

	return AI::ECommandStatus::Running;
}
//---------------------------------------------------------------------

static AI::ECommandStatus InteractWithTarget(CGameSession& Session, CAbilityInstance& AbilityInstance, HEntity EntityID, float dt)
{
	// Start interaction, if not yet
	if (AbilityInstance.Stage != EAbilityExecutionStage::Interaction)
	{
		//!!!TODO: if animation graph override is defined, enable it!

		//???after OnStart should immediately update for the part of dt not used by movement and facing?
		AbilityInstance.PrevElapsedTime = 0.f;
		AbilityInstance.ElapsedTime = 0.f;

		AbilityInstance.Stage = EAbilityExecutionStage::Interaction;
		AbilityInstance.Ability.OnStart(Session, AbilityInstance);
	}
	else
	{
		AbilityInstance.PrevElapsedTime = AbilityInstance.ElapsedTime;
		AbilityInstance.ElapsedTime += dt;
	}

	return AbilityInstance.Ability.OnUpdate(Session, AbilityInstance);
}
//---------------------------------------------------------------------

static void FinalizeCommands(CGameSession& Session, AI::CCommandStackComponent& CmdStack)
{
	CmdStack.FinalizePoppedCommands<ExecuteAbility>([&Session](ExecuteAbility& Cmd, AI::ECommandStatus Status)
	{
		//!!!if animation graph override is enabled, disable it!

		if (Cmd._AbilityInstance)
		{
			if (Cmd._AbilityInstance->Stage == EAbilityExecutionStage::Interaction)
				Cmd._AbilityInstance->Ability.OnEnd(Session, *Cmd._AbilityInstance, Status);

			Cmd._AbilityInstance = nullptr;
		}

		// This sub-command is already popped too and needs no cancellation request, but
		// if not cleared here, ExecuteAbility's own future may hold the whole chain in memory
		Cmd._SubCommandFuture = {};
	});
}
//---------------------------------------------------------------------

static AI::ECommandStatus ProcessAgentAbility(CGameSession& Session, Game::HEntity EntityID, AI::CCommandStackComponent& CmdStack, AI::CCommandStackHandle Cmd, float dt)
{
	// No need in further processing if no command received
	if (!Cmd) return AI::ECommandStatus::Succeeded;

	// Fulfil cancellation request
	if (Cmd->GetStatus() == AI::ECommandStatus::Cancelled) return AI::ECommandStatus::Cancelled;

	n_assert2_dbg(!IsFinishedCommandStatus(Cmd->GetStatus()), "Only the ability execution system itself might set an ExecuteAbility command finished");

	auto* pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return AI::ECommandStatus::Failed;

	auto* pTypedCmd = Cmd->As<ExecuteAbility>();

	// Succeed immediately if there is no ability to execute. This should never happen.
	n_assert_dbg(pTypedCmd->_AbilityInstance);
	if (!pTypedCmd->_AbilityInstance) return AI::ECommandStatus::Succeeded;

	auto& AbilityInstance = *pTypedCmd->_AbilityInstance;

	// Start executing a new command
	const bool IsNewCommand = (Cmd->GetStatus() == AI::ECommandStatus::NotStarted);
	if (IsNewCommand)
	{
		Cmd->SetStatus(AI::ECommandStatus::Running);

		// Initialize new ability
		AbilityInstance.Actor = EntityID;
		AbilityInstance.Stage = EAbilityExecutionStage::Movement;

		auto& Zones = AbilityInstance.InitialZones;

		// Get interaction zones from a smart object, if present
		if (!AbilityInstance.Targets.empty())
		{
			if (auto* pSOComponent = pWorld->FindComponent<const CSmartObjectComponent>(AbilityInstance.Targets[0].Entity))
			{
				// Get zones from entity component
				Zones.reserve(pSOComponent->Zones.size());
				for (U8 i = 0; i < pSOComponent->Zones.size(); ++i)
					Zones.push_back(&pSOComponent->Zones[i]);

				// Get zones from asset
				auto* pSOAsset = pSOComponent->Asset ? pSOComponent->Asset->GetObject<CSmartObject>() : nullptr;
				if (pSOAsset)
				{
					// TODO: can check additional conditions of zone before adding it!
					const auto ZoneCount = pSOAsset->GetInteractionZoneCount();
					Zones.reserve(Zones.size() + ZoneCount);
					for (U8 i = 0; i < ZoneCount; ++i)
						Zones.push_back(&pSOAsset->GetInteractionZone(i));
				}
			}
		}

		// Get interaction zones from an ability
		AbilityInstance.Ability.GetZones(Session, *pTypedCmd->_AbilityInstance, Zones);
		n_assert_dbg(!Zones.empty());
	}

	// Terminate an ability if its logic requested it
	const auto RequestedStatus = AbilityInstance.RequestedStatus;
	if (AI::IsTerminalCommandStatus(RequestedStatus)) return RequestedStatus;

	if (Cmd->IsChanged())
	{
		// Ability execution command should not change on the fly
		NOT_IMPLEMENTED;
		Cmd->AcceptChanges();
	}

	// Determine target node, if any
	Scene::CSceneNode* pTargetRootNode = nullptr;
	if (!AbilityInstance.Targets.empty())
	{
		const auto& Target = AbilityInstance.Targets[0];
		if (auto* pTargetSceneComponent = pWorld->FindComponent<CSceneComponent>(Target.Entity))
			pTargetRootNode = pTargetSceneComponent->RootNode;
		else
			pTargetRootNode = Target.pNode;
	}

	// Update main target transform
	bool TargetTransformChanged = false;
	if (IsNewCommand)
	{
		TargetTransformChanged = true;
		if (pTargetRootNode)
		{
			//???store temporary system fields in a command instead of ability instance?
			AbilityInstance.PrevTargetTfmVersion = pTargetRootNode->GetTransformVersion();
			AbilityInstance.TargetToWorld = pTargetRootNode->GetWorldMatrix();
		}
		else if (!AbilityInstance.Targets.empty())
		{
			// FIXME: AbilityInstance.TargetToWorld not needed if CTargetInfo will store full SRT instead of Point!
			AbilityInstance.TargetToWorld.w_axis = AbilityInstance.Targets[0].Point;
		}
	}
	else if (pTargetRootNode && AbilityInstance.PrevTargetTfmVersion != pTargetRootNode->GetTransformVersion())
	{
		TargetTransformChanged = true;
		AbilityInstance.PrevTargetTfmVersion = pTargetRootNode->GetTransformVersion();
		AbilityInstance.TargetToWorld = pTargetRootNode->GetWorldMatrix();
	}

	// Ability execution logic

	AI::ECommandStatus Result = MoveToTarget(Session, AbilityInstance, *pWorld, EntityID, CmdStack, *pTypedCmd, TargetTransformChanged);

	if (Result == AI::ECommandStatus::Succeeded)
		Result = FaceTarget(Session, AbilityInstance, *pWorld, EntityID, CmdStack, *pTypedCmd, TargetTransformChanged);

	if (Result == AI::ECommandStatus::Succeeded)
		Result = InteractWithTarget(Session, AbilityInstance, EntityID, dt);

	return Result;
}
//---------------------------------------------------------------------

void UpdateAbilityInteractions(CGameSession& Session, CGameWorld& World, float dt)
{
	World.ForEachEntityWith<AI::CCommandStackComponent, AI::CAIStateComponent>(
		[&Session, &World, dt](auto EntityID, auto& Entity, AI::CCommandStackComponent& CmdStack, AI::CAIStateComponent& AIState)
	{
		// Finalize commands popped since the system was executed the last time
		FinalizeCommands(Session, CmdStack);

		// Pick a supported command to process
		const auto Cmd = CmdStack.FindTopmostCommand<ExecuteAbility>();

		// Do the main job of the system
		const auto Status = ProcessAgentAbility(Session, EntityID, CmdStack, Cmd, dt);

		// Process finished or failed commands
		if (Cmd && AI::IsTerminalCommandStatus(Status))
		{
			CmdStack.PopCommand(Cmd, Status);
			FinalizeCommands(Session, CmdStack);
		}
	});
}
//---------------------------------------------------------------------

}
