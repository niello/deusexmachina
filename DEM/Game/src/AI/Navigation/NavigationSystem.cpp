#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <Game/ECS/Components/SceneComponent.h>
#include <Game/GameLevel.h>
#include <AI/Navigation/NavAgentComponent.h>
#include <AI/Navigation/NavControllerComponent.h>
#include <AI/Navigation/PathRequestQueue.h>
#include <AI/Navigation/TraversalAction.h>
#include <AI/Navigation/NavMeshDebugDraw.h>
#include <AI/Navigation/NavMesh.h>
#include <Debug/DebugDraw.h>
#include <DetourCommon.h>
#include <DetourDebugDraw.h>
#include <array>

namespace DEM::AI
{

constexpr float EQUALITY_THRESHOLD_SQ = 1.0f / (16384.0f * 16384.0f);
static inline bool InRange(const float* p0, const float* p1, float AgentHeight, float SqRange)
{
	return dtVdist2DSqr(p0, p1) <= SqRange && std::abs(p0[1] - p1[1]) < AgentHeight;
}
//---------------------------------------------------------------------

// Returns whether an agent can continue to perform the current navigation task
static bool UpdatePosition(const vector3& Position, CNavAgentComponent& Agent)
{
	const auto* pNavFilter = Agent.Settings->GetQueryFilter();
	const float RecoveryRadius = std::min(Agent.Radius * 2.f, 20.f);

	// When already recovering, reduce threshold 10 times to avoid jitter on the border
	const float SqRecoveryThreshold = Agent.Radius * Agent.Radius *
		((Agent.Mode == ENavigationMode::Recovery) ? 0.0001f : 0.01f);

	// Check if we are moving
	// NB: recovery movement may result in actiual position == corridor position, don't check for inequality in that case
	if (Agent.State != ENavigationState::Idle &&
		(Agent.Mode == ENavigationMode::Recovery ||
			(Agent.Mode == ENavigationMode::Surface &&
			 !InRange(Agent.Corridor.getPos(), Position.v, Agent.Height, EQUALITY_THRESHOLD_SQ))))
	{
		const auto PrevPoly = Agent.Corridor.getFirstPoly();

		// Agent moves along the navmesh surface or recovers to it, adjust the corridor
		if (Agent.Corridor.movePosition(Position.v, Agent.pNavQuery, pNavFilter))
		{
			const float SqDeviationFromCorridor = dtVdist2DSqr(Position.v, Agent.Corridor.getPos());
			if (SqDeviationFromCorridor <= SqRecoveryThreshold)
			{
				// Close enough to be considered being on the navmesh
				Agent.Mode = ENavigationMode::Surface;
				if (PrevPoly != Agent.Corridor.getFirstPoly())
					Agent.pNavQuery->getAttachedNavMesh()->getPolyArea(Agent.Corridor.getFirstPoly(), &Agent.CurrAreaType);
				return true;
			}
			else if (SqDeviationFromCorridor <= RecoveryRadius * RecoveryRadius)
			{
				// Recover to the corridor start to prevent tunneling
				Agent.Mode = ENavigationMode::Recovery;
				Agent.CurrAreaType = 0;
				return true;
			}
		}
	}
	else
	{
		// Agent is idle or moves along the offmesh connection, check the current poly validity
		if (Agent.pNavQuery->isValidPolyRef(Agent.Corridor.getFirstPoly(), pNavFilter)) return true;

		// If offmesh connection became invalid, cancel its traversal and fail navigation task
		if (Agent.Mode == ENavigationMode::Offmesh) return false;
	}

	// Our current poly is unknown or invalid, find the nearest valid one
	const float RecoveryExtents[3] = { RecoveryRadius, Agent.Height, RecoveryRadius };
	dtPolyRef NearestRef = 0;
	float NearestPos[3];
	Agent.pNavQuery->findNearestPoly(Position.v, RecoveryExtents, pNavFilter, &NearestRef, NearestPos);

	if (!NearestRef)
	{
		// No poly found in a recovery radius, agent can't recover and needs external position change
		Agent.Corridor.reset(0, Position.v);
		return false;
	}

	// Check if our new location is far enough from the navmesh to enable recovery mode
	if (dtVdist2DSqr(Position.v, NearestPos) > SqRecoveryThreshold)
	{
		Agent.Mode = ENavigationMode::Recovery;
		Agent.CurrAreaType = 0;
	}
	else
	{
		Agent.Mode = ENavigationMode::Surface;
		Agent.pNavQuery->getAttachedNavMesh()->getPolyArea(NearestRef, &Agent.CurrAreaType);
	}

	if (Agent.State == ENavigationState::Idle)
	{
		Agent.Corridor.reset(NearestRef, NearestPos);
	}
	else if (Agent.Corridor.getFirstPoly() != NearestRef)
	{
		// TODO: if this doesn't work as intended, may implement the following logic:
		// 1. Check if the nearest poly is one on our path (or only 1st poly in the corridor, or 1st and 2nd)
		// 2. If yes, keep corridor without replanning and recover to the nearest pos
		// 3. Else reset corridor to the new position and replan
		// Also can recover to the corridor poly that is valid and most close to start.
		// Must handle possible clipping through thin impassable areas.

		//!!!FIXME: check if offmesh is being traversed and first corridor poly is dest, not curr!
		n_assert_dbg(Agent.Mode != ENavigationMode::Offmesh);

		// Make sure the first polygon is valid, but leave other valid
		// polygons in the path so that replanner can adjust the path better.
		Agent.Corridor.fixPathStart(NearestRef, NearestPos);
		Agent.State = ENavigationState::Requested;
	}

	return true;
}
//---------------------------------------------------------------------

// Returns whether an agent can continue to perform the current navigation task
static bool UpdateDestination(Navigate& Action, CNavAgentComponent& Agent, ::AI::CPathRequestQueue& PathQueue)
{
	//!!!FIXME: setting, per Navigate action or per entity!
	constexpr float MaxTargetOffset = 0.5f;
	constexpr float MaxTargetOffsetSq = MaxTargetOffset * MaxTargetOffset;

	const auto* pNavFilter = Agent.Settings->GetQueryFilter();

	if (Agent.State != ENavigationState::Idle)
	{
		if (InRange(Agent.TargetPos.v, Action._Destination.v, Agent.Height, EQUALITY_THRESHOLD_SQ))
		{
			// Target remains static but we must check its poly validity anyway
			if (Agent.pNavQuery->isValidPolyRef(Agent.TargetRef, pNavFilter)) return true;
		}
		else if (Agent.State == ENavigationState::Following)
		{
			// Planning finished, can adjust moving target in the corridor
			if (Agent.Corridor.moveTargetPosition(Action._Destination.v, Agent.pNavQuery, pNavFilter))
			{
				// Check if target matches the corridor or moved not too far from it into impassable zone.
				// Otherwise target is considered teleported and may require replanning (see below).
				const auto OffsetSq = dtVdist2DSqr(Action._Destination.v, Agent.Corridor.getTarget());
				if (OffsetSq < EQUALITY_THRESHOLD_SQ ||
					(Agent.TargetRef == Agent.Corridor.getLastPoly() && OffsetSq <= MaxTargetOffsetSq))
				{
					Agent.TargetRef = Agent.Corridor.getLastPoly();
					Agent.TargetPos = Agent.Corridor.getTarget();
					return true;
				}
			}
		}
	}

	// Target poly is unknown or invalid, find the nearest valid one
	const float TargetExtents[3] = { MaxTargetOffset, Agent.Height, MaxTargetOffset };
	Agent.pNavQuery->findNearestPoly(Action._Destination.v, TargetExtents, pNavFilter, &Agent.TargetRef, Agent.TargetPos.v);

	// If no poly found in a target radius, navigation task is failed
	if (!Agent.TargetRef || dtVdist2DSqr(Action._Destination.v, Agent.TargetPos.v) > MaxTargetOffsetSq)
		return false;

	// If target is in the current corridor, can avoid replanning
	if (Agent.State != ENavigationState::Idle)
	{
		for (int i = 0; i < Agent.Corridor.getPathCount(); ++i)
			if (Agent.Corridor.getPath()[i] == Agent.TargetRef)
			{
				Agent.Corridor.shrink(Agent.TargetPos.v, i + 1);
				if (Agent.AsyncTaskID)
				{
					PathQueue.CancelRequest(Agent.AsyncTaskID);
					Agent.AsyncTaskID = 0;
				}
				Agent.State = ENavigationState::Following;
				return true;
			}
	}

	Agent.State = ENavigationState::Requested;
	return true;
}
//---------------------------------------------------------------------

static bool CheckCurrentPath(CNavAgentComponent& Agent)
{
	// No path - no replanning
	if (Agent.State == ENavigationState::Requested || Agent.State == ENavigationState::Idle) return true;

	// If nearby corridor is not valid, replan
	constexpr int CHECK_LOOKAHEAD = 10;
	if (!Agent.Corridor.isValid(CHECK_LOOKAHEAD, Agent.pNavQuery, Agent.Settings->GetQueryFilter())) return false;

	// If the end of the path is near and it is not the requested location, replan
	constexpr float TARGET_REPLAN_DELAY = 1.0f; // seconds
	if (Agent.State == ENavigationState::Following &&
		Agent.ReplanTime > TARGET_REPLAN_DELAY &&
		Agent.Corridor.getPathCount() < CHECK_LOOKAHEAD &&
		Agent.Corridor.getLastPoly() != Agent.TargetRef)
	{
		return false;
	}

	return true;
}
//---------------------------------------------------------------------

static void RequestPath(CNavAgentComponent& Agent, ::AI::CPathRequestQueue& PathQueue)
{
	const auto* pNavFilter = Agent.Settings->GetQueryFilter();

	// Quick synchronous search towards the goal, limited iteration count
	Agent.pNavQuery->initSlicedFindPath(Agent.Corridor.getFirstPoly(), Agent.TargetRef, Agent.Corridor.getPos(), Agent.TargetPos.v, pNavFilter);
	Agent.pNavQuery->updateSlicedFindPath(20, 0);

	// Try to reuse existing steady path if possible and worthwile
	std::array<dtPolyRef, 32> Path;
	int PathSize = 0;
	dtStatus Status = (Agent.Corridor.getPathCount() > 10) ?
		Agent.pNavQuery->finalizeSlicedFindPathPartial(Agent.Corridor.getPath(), Agent.Corridor.getPathCount(), Path.data(), &PathSize, Path.size()) :
		Agent.pNavQuery->finalizeSlicedFindPath(Path.data(), &PathSize, Path.size());

	// Setup path corridor
	const dtPolyRef LastPoly = (dtStatusFailed(Status) || !PathSize) ? 0 : Path[PathSize - 1];
	if (LastPoly == Agent.TargetRef)
	{
		// Full path
		Agent.Corridor.setCorridor(Agent.TargetPos.v, Path.data(), PathSize);
		Agent.State = ENavigationState::Following;
		Agent.ReplanTime = 0.f;
	}
	else
	{
		// Partial path, constrain target position inside the last polygon or reset to agent position
		float PartialTargetPos[3];
		if (LastPoly && dtStatusSucceed(Agent.pNavQuery->closestPointOnPolyBoundary(LastPoly, Agent.TargetPos.v, PartialTargetPos)))
			Agent.Corridor.setCorridor(PartialTargetPos, Path.data(), PathSize);
		else
			Agent.Corridor.setCorridor(Agent.Corridor.getPos(), Agent.Corridor.getPath(), 1);

		// Request async path planning
		if (Agent.AsyncTaskID) PathQueue.CancelRequest(Agent.AsyncTaskID);
		Agent.AsyncTaskID = PathQueue.Request(Agent.Corridor.getLastPoly(), Agent.TargetRef, Agent.Corridor.getTarget(), Agent.TargetPos.v, Agent.pNavQuery, pNavFilter);
		if (Agent.AsyncTaskID) Agent.State = ENavigationState::Planning;
	}
}
//---------------------------------------------------------------------

static bool CheckAsyncPathResult(CNavAgentComponent& Agent, ::AI::CPathRequestQueue& PathQueue)
{
	dtStatus Status = PathQueue.GetRequestStatus(Agent.AsyncTaskID);

	// If failed, can retry because the target location is still valid
	if (dtStatusFailed(Status)) Agent.State = ENavigationState::Requested;
	else if (!dtStatusSucceed(Status)) return true;

	std::array<dtPolyRef, 512> Path;
	int PathSize = 0;
	n_assert(PathQueue.GetPathSize(Agent.AsyncTaskID) <= static_cast<int>(Path.size()));
	Status = PathQueue.GetPathResult(Agent.AsyncTaskID, Path.data(), PathSize, Path.size());

	// The last ref in the old path should be the same as the location where the request was issued
	if (dtStatusFailed(Status) || !PathSize || Agent.Corridor.getLastPoly() != Path[0]) return false;

	// If path is partial, try to constrain target position inside the last polygon
	vector3 PathTarget = Agent.TargetPos;
	const auto LastRef = Path[PathSize - 1];
	if (LastRef != Agent.TargetRef)
		if (dtStatusFailed(Agent.pNavQuery->closestPointOnPolyBoundary(LastRef, Agent.TargetPos.v, PathTarget.v)))
			return false;

	// Merge result and existing path, removing trackbacks. The agent might have moved whilst
	// the request is being processed, so the path may have changed.
	int TotalSize = 0;
	Status = Agent.Corridor.appendPath(PathTarget.v, Path.data(), PathSize, &TotalSize);
	if (dtStatusDetail(Status, DT_BUFFER_TOO_SMALL))
	{
		Agent.Corridor.extend(TotalSize);
		Status = Agent.Corridor.appendPath(PathTarget.v, Path.data(), PathSize, &TotalSize);
	}

	if (dtStatusFailed(Status)) return false;

	Agent.State = ENavigationState::Following;
	Agent.ReplanTime = 0.f;
	return true;
}
//---------------------------------------------------------------------

// Returns OutNextTurn for corridor visibility optimization
static bool GenerateTraversalAction(Game::CGameWorld& World, CNavAgentComponent& Agent, Game::CActionQueueComponent& Queue,
	Game::HAction NavAction, const vector3& ExactPos, vector3& OutNextTurn)
{
	// Check if we are able to trigger an offmesh connection at the next corner
	//???add shortcut method to corridor? Agent.Corridor.initStraightPathSearch(Agent.pNavQuery, Ctx);
	dtStraightPathContext Ctx;
	if (dtStatusFailed(Agent.pNavQuery->initStraightPathSearch(
		Agent.Corridor.getPos(), Agent.Corridor.getTarget(), Agent.Corridor.getPath(), Agent.Corridor.getPathCount(), Ctx)))
		return false;

	// Offmesh start is always a corner, so ignore area and poly changes on the way
	unsigned char Flags;
	unsigned char AreaType;
	dtPolyRef PolyRef;
	if (dtStatusFailed(Agent.pNavQuery->findNextStraightPathPoint(Ctx, OutNextTurn.v, &Flags, &AreaType, &PolyRef, 0))) return false;

	Game::HEntity Controller;

	// If it is an offmesh connection, we may start its traversal
	if (Flags & DT_STRAIGHTPATH_OFFMESH_CONNECTION)
	{
		// Check if we can traverse this connection
		if (auto pAction = Agent.Settings->FindAction(World, Agent, AreaType, PolyRef, &Controller))
		{
			const float OffmeshRadius = Agent.NavMap->GetDetourNavMesh()->getOffMeshConnectionByRef(PolyRef)->rad;

			// Check if we are in a trigger range
			if (std::abs(ExactPos.y - OutNextTurn.y) < Agent.Height &&
				vector3::SqDistance2D(ExactPos, OutNextTurn) < pAction->GetSqTriggerRadius(Agent.Radius, OffmeshRadius))
			{
				dtPolyRef OffmeshRefs[2];
				vector3 Start, End;
				if (Agent.Corridor.moveOverOffmeshConnection(PolyRef, OffmeshRefs, Start.v, End.v, Agent.pNavQuery))
				{
					// Generate action with precalculated path edge
					if (!pAction->GenerateAction(World, Agent, Controller, Queue, NavAction, ExactPos, Start, End))
						return false;

					Agent.Mode = ENavigationMode::Offmesh;
					Agent.OffmeshRef = PolyRef;
					Agent.CurrAreaType = AreaType;
					return true;
				}
			}
		}
	}

	// We triggered no offmesh connection and have no precalculated path edge, let's traverse navmesh surface
	auto pAction = Agent.Settings->FindAction(World, Agent, Agent.CurrAreaType, Agent.Corridor.getFirstPoly(), &Controller);
	return pAction && pAction->GenerateAction(World, Agent, Controller, Queue, NavAction, ExactPos);
}
//---------------------------------------------------------------------

static void ResetNavigation(CNavAgentComponent& Agent, ::AI::CPathRequestQueue& PathQueue,
	Game::CActionQueueComponent& Queue, Game::HAction NavAction, Game::EActionStatus Result)
{
	if (Agent.AsyncTaskID)
	{
		PathQueue.CancelRequest(Agent.AsyncTaskID);
		Agent.AsyncTaskID = 0;
	}

	if (auto CurrPoly = Agent.Corridor.getFirstPoly())
		Agent.Corridor.reset(CurrPoly, Agent.Corridor.getPos());

	Agent.State = ENavigationState::Idle;
	if (Agent.Mode == ENavigationMode::Offmesh)
		Agent.Mode = ENavigationMode::Recovery;

	Queue.SetStatus(NavAction, Result);
}
//---------------------------------------------------------------------

void InitNavigationAgents(Game::CGameWorld& World, Game::CGameLevel& Level, Resources::CResourceManager& ResMgr)
{
	World.ForEachComponent<DEM::AI::CNavAgentComponent>([&Level, &ResMgr](auto EntityID, DEM::AI::CNavAgentComponent& NavAgent)
	{
		auto Rsrc = ResMgr.RegisterResource<DEM::AI::CNavAgentSettings>(NavAgent.SettingsID.CStr());
		NavAgent.Settings = Rsrc->ValidateObject<DEM::AI::CNavAgentSettings>();

		NavAgent.Corridor.init(256);

		NavAgent.NavMap = Level.GetNavMap(NavAgent.Radius, NavAgent.Height);
		if (NavAgent.NavMap)
		{
			//!!!???pooled queries? don't allocate one per agent?!
			NavAgent.pNavQuery = dtAllocNavMeshQuery();
			NavAgent.pNavQuery->init(NavAgent.NavMap->GetDetourNavMesh(), 512);
		}
	});
}
//---------------------------------------------------------------------

void ProcessNavigation(DEM::Game::CGameWorld& World, float dt, ::AI::CPathRequestQueue& PathQueue, bool NewFrame)
{
	World.ForEachEntityWith<CNavAgentComponent, DEM::Game::CActionQueueComponent, const DEM::Game::CSceneComponent>(
		[dt, &World, &PathQueue, NewFrame](auto EntityID, auto& Entity,
			CNavAgentComponent& Agent,
			DEM::Game::CActionQueueComponent* pQueue,
			const DEM::Game::CSceneComponent* pSceneComponent)
	{
		if (!pSceneComponent->RootNode || !Agent.pNavQuery || !Agent.Settings) return;

		const auto PrevMode = Agent.Mode;

		// Update navigation status from the curent agent position
		const auto& Pos = pSceneComponent->RootNode->GetWorldPosition();
		if (!UpdatePosition(Pos, Agent))
		{
			ResetNavigation(Agent, PathQueue, *pQueue, pQueue->FindActive<Navigate>(), DEM::Game::EActionStatus::Failed);
			return;
		}

		auto NavigateAction = pQueue->FindActive<Navigate>();
		if (auto pNavigateAction = NavigateAction.As<Navigate>())
		{
			const auto SubActionStatus = pQueue->GetStatus(pQueue->GetChild(NavigateAction));
			const bool HasSubAction = (SubActionStatus != Game::EActionStatus::NotQueued);

			// Check if sub-action has finished
			if (HasSubAction)
			{
				if (SubActionStatus == DEM::Game::EActionStatus::Failed || SubActionStatus == DEM::Game::EActionStatus::Cancelled)
				{
					// If sub-action failed or cancelled, finish navigation with the same result
					ResetNavigation(Agent, PathQueue, *pQueue, NavigateAction, SubActionStatus);
					return;
				}
				else if (SubActionStatus == DEM::Game::EActionStatus::Succeeded)
				{
					// If the last edge traversed successfully, finish navigation
					// FIXME: looks like hack, can rewrite better? Could compare curr and target polys? Action can't change inside a poly.
					// Problems: Steer to door knob? Same poly, different action, or sub-sub-action (then OK)? Traverse ofm, too early dest poly, look at curr Mode?
					if (Agent.IsTraversingLastEdge)
					{
						ResetNavigation(Agent, PathQueue, *pQueue, NavigateAction, DEM::Game::EActionStatus::Succeeded);
						return;
					}

					// Finish traversing an offmesh connection
					if (Agent.Mode == ENavigationMode::Offmesh)
					{
						Agent.Mode = ENavigationMode::Surface;
						Agent.pNavQuery->getAttachedNavMesh()->getPolyArea(Agent.Corridor.getFirstPoly(), &Agent.CurrAreaType);
					}
				}
			}

			// Multiple physics frames can be processed inside one logic frame. Target remains the same
			// during the logic frame but might be reached by physics in the middle of it. So if there
			// is an active sub-action, let's just continue executing it, without unnecessary update.
			if (!NewFrame && PrevMode == Agent.Mode && HasSubAction) return;

			// NB: in Detour replan time increases only when on navmesh (not offmesh, not invalid)
			Agent.ReplanTime += dt;

			// Process target location changes and validity
			if (!UpdateDestination(*pNavigateAction, Agent, PathQueue))
			{
				ResetNavigation(Agent, PathQueue, *pQueue, NavigateAction, DEM::Game::EActionStatus::Failed);
				return;
			}

			// Check current path validity, replan if can't continue using it
			if (!CheckCurrentPath(Agent))
				Agent.State = ENavigationState::Requested;

			// Do async path planning
			if (Agent.State == ENavigationState::Requested)
			{
				RequestPath(Agent, PathQueue);
			}
			else if (Agent.State == ENavigationState::Planning)
			{
				if (!CheckAsyncPathResult(Agent, PathQueue))
				{
					ResetNavigation(Agent, PathQueue, *pQueue, NavigateAction, DEM::Game::EActionStatus::Failed);
					return;
				}
			}

			// Generate sub-action for path following
			if (Agent.Mode == ENavigationMode::Recovery)
			{
				// Try to return to the navmesh by the shortest path
				auto pAction = Agent.Settings->FindAction(World, Agent, 0, 0, nullptr);
				if (!pAction || !pAction->GenerateRecoveryAction(*pQueue, NavigateAction, Agent.Corridor.getPos()))
					ResetNavigation(Agent, PathQueue, *pQueue, NavigateAction, DEM::Game::EActionStatus::Failed);
			}
			else if (Agent.Mode == ENavigationMode::Surface)
			{
				const auto* pNavFilter = Agent.Settings->GetQueryFilter();

				Agent.PathOptimizationTime += dt;

				// FIXME: only if necessary events happened? like offsetting from expected position.
				// The more inaccurate the agent movement, the more beneficial this function becomes. (c) Docs
				// The same for optimizePathVisibility (used below).
				constexpr float OPT_TIME_THR_SEC = 2.5f;
				const bool OptimizePath = (Agent.PathOptimizationTime >= OPT_TIME_THR_SEC);
				if (OptimizePath)
				{
					Agent.Corridor.optimizePathTopology(Agent.pNavQuery, pNavFilter);
					Agent.PathOptimizationTime = 0.f;
				}

				vector3 NextTurn;
				if (!GenerateTraversalAction(World, Agent, *pQueue, NavigateAction, Pos, NextTurn))
					ResetNavigation(Agent, PathQueue, *pQueue, NavigateAction, DEM::Game::EActionStatus::Failed);

				// Optimize path visibility along the [corridor pos -> NextTurn] straight line
				if (OptimizePath && Agent.Mode == ENavigationMode::Surface)
					Agent.Corridor.optimizePathVisibility(NextTurn.v, 30.f * Agent.Radius, Agent.pNavQuery, pNavFilter);
			}
			else if (/*Agent.Mode == ENavigationMode::Offmesh &&*/ !HasSubAction)
			{
				//???can optimize all traversal cases by storing action and comparing new with current?
				//if so, can avoid storing curr offmesh poly in this case, and maybe surface traversal will benefit too.
				Game::HEntity Controller;
				auto pAction = Agent.Settings->FindAction(World, Agent, Agent.CurrAreaType, Agent.OffmeshRef, &Controller);
				if (!pAction || !pAction->GenerateAction(World, Agent, Controller, *pQueue, NavigateAction, Pos))
					ResetNavigation(Agent, PathQueue, *pQueue, NavigateAction, DEM::Game::EActionStatus::Failed);
			}
		}
		else if (Agent.State != ENavigationState::Idle)
		{
			ResetNavigation(Agent, PathQueue, *pQueue, NavigateAction, DEM::Game::EActionStatus::Cancelled);
		}
	});
}
//---------------------------------------------------------------------

void RenderDebugNavigation(DEM::Game::CGameWorld& World, Debug::CDebugDraw& DebugDraw)
{
	World.ForEachEntityWith<const CNavAgentComponent, const DEM::Game::CSceneComponent>(
		[&DebugDraw](auto EntityID, auto& Entity,
			const CNavAgentComponent& Agent,
			const DEM::Game::CSceneComponent* pSceneComponent)
	{
		if (!pSceneComponent->RootNode || !Agent.pNavQuery) return;

		if (Agent.State == ENavigationState::Planning || Agent.State == ENavigationState::Following)
		{
			constexpr auto ColorPathLine = Render::ColorRGBANorm(1.f, 0.75f, 0.5f, 1.f);
			constexpr auto ColorPathCorner = Render::ColorRGBANorm(1.f, 0.9f, 0.f, 1.f);
			constexpr auto ColorPoly = Render::ColorRGBA(255, 196, 0, 64);

			const float* pCurrPos = Agent.Corridor.getPos();
			const auto* pCurrPath = Agent.Corridor.getPath();

			// Path polys
			Debug::CNavMeshDebugDraw DD(DebugDraw);
			for (int i = 0; i < Agent.Corridor.getPathCount(); ++i)
				duDebugDrawNavMeshPoly(&DD, *Agent.pNavQuery->getAttachedNavMesh(), pCurrPath[i], ColorPoly);

			// Straight path
			dtStraightPathContext Ctx;
			if (dtStatusSucceed(Agent.pNavQuery->initStraightPathSearch(
				pCurrPos, Agent.Corridor.getTarget(), pCurrPath, Agent.Corridor.getPathCount(), Ctx)))
			{
				vector3 From = pSceneComponent->RootNode->GetWorldPosition();
				vector3 To;
				dtStatus Status;
				U8 AreaType = Agent.CurrAreaType;
				do
				{
					const int Options = Agent.Settings->IsAreaControllable(AreaType) ? DT_STRAIGHTPATH_ALL_CROSSINGS : DT_STRAIGHTPATH_AREA_CROSSINGS;
					Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, To.v, nullptr, &AreaType, nullptr, Options);
					if (dtStatusFailed(Status)) break;

					DebugDraw.DrawLine(From, To, ColorPathLine);
					DebugDraw.DrawPoint(To, ColorPathCorner, 6.f);
					From = To;
				}
				while (dtStatusInProgress(Status));
			}
		}

		/*
		const char* pNavStr = nullptr;
		if (pActor->NavState == AINav_Done) pNavStr = "Done";
		else if (pActor->NavState == AINav_Failed) pNavStr = "Failed";
		else if (pActor->NavState == AINav_DestSet) pNavStr = "DestSet";
		else if (pActor->NavState == AINav_Planning) pNavStr = "Planning";
		else if (pActor->NavState == AINav_Following) pNavStr = "Following";

		CString Text;
		Text.Format(
			"Nav state: %s\n"
			"Nav location is %s\n"
			"Curr poly: %d\n"
			"Dest poly: %d\n"
			"Destination: %.4f, %.4f, %.4f\n",
			pNavStr,
			pActor->IsNavLocationValid() ? "valid" : "invalid",
			Corridor.getFirstPoly(),
			DestRef,
			DestPoint.x, DestPoint.y, DestPoint.z);
		//DebugDraw->DrawText(Text.CStr(), 0.65f, 0.1f);
		*/
	});
}
//---------------------------------------------------------------------

void DestroyNavigation(DEM::Game::CGameWorld& World, ::AI::CPathRequestQueue& PathQueue)
{
	World.RemoveAllComponents<DEM::AI::CNavAgentComponent>();

	World.FreeDead<DEM::AI::CNavAgentComponent>([&World, &PathQueue](auto EntityID, DEM::AI::CNavAgentComponent& Agent)
	{
		// Cancel active navigation tasks
		if (auto pQueue = World.FindComponent<Game::CActionQueueComponent>(EntityID))
		{
			Game::HAction TopNavAction;
			while (auto NavAction = pQueue->FindActive<Navigate>(TopNavAction))
				TopNavAction = NavAction;

			if (TopNavAction && pQueue->GetStatus(TopNavAction) == Game::EActionStatus::Active)
				pQueue->SetStatus(TopNavAction, Game::EActionStatus::Cancelled);
		}

		// Need to shutdown async tasks before deleting agent components
		if (Agent.AsyncTaskID)
		{
			PathQueue.CancelRequest(Agent.AsyncTaskID);
			Agent.AsyncTaskID = 0;
		}

		if (Agent.pNavQuery)
		{
			dtFreeNavMeshQuery(Agent.pNavQuery);
			Agent.pNavQuery = nullptr;
		}
	});
}
//---------------------------------------------------------------------

// TODO: unregister!!!
void RegisterNavigationControllers(DEM::Game::CGameWorld& World)
{
	World.ForEachEntityWith<const CNavControllerComponent>(
		[&World](auto EntityID, auto& Entity, const CNavControllerComponent& Component)
	{
		if (auto pLevel = World.FindLevel(Entity.LevelID))
			pLevel->SetNavRegionController(Component.RegionID, EntityID);
	});
}
//---------------------------------------------------------------------

}

/*

dtPolyRef CNavSystem::GetNearestPoly(dtPolyRef* pPolys, int PolyCount, vector3& OutPos) const
{
	// Optimization: if one of polys found is a poly we are on, select it
	dtPolyRef Ref = Corridor.getFirstPoly();
	if (Ref)
		for (int i = 0; i < PolyCount; ++i)
			if (pPolys[i] == Ref)
			{
				dtVcopy(OutPos.v, Corridor.getPos());
				return Ref;
			}

	dtPolyRef NearestPoly = 0;

	//!!!Copied from Detour dtNavMeshQuery::findNearestPoly
	//???to utility function dtNavMeshQuery::getNearestPoly(Polys, Point)? ask memononen
	float* pPos = pActor->Position.v;
	float MinSqDist = FLT_MAX;
	for (int i = 0; i < PolyCount; ++i)
	{
		dtPolyRef NearRef = pPolys[i];
		float ClosestPtPoly[3];
		bool IsOverPoly;
		pNavQuery->closestPointOnPoly(NearRef, pPos, ClosestPtPoly, &IsOverPoly);
		float SqDist = dtVdistSqr(pPos, ClosestPtPoly); //???dtVdist2DSqr
		if (SqDist < MinSqDist)
		{
			MinSqDist = SqDist;
			NearestPoly = NearRef;
			dtVcopy(OutPos.v, ClosestPtPoly);
		}
	}

	return NearestPoly;
}
//---------------------------------------------------------------------

UPTR CNavSystem::GetValidPolys(const vector3& Center, float MinRange, float MaxRange, CArray<dtPolyRef>& Polys) const
{
	if (!pNavQuery || MinRange > MaxRange) return 0;

	const float SqMinRange = MinRange * MinRange;
	const float SqMaxRange = MaxRange * MaxRange;

	dtPolyRef Ref;
	vector3 Pt;
	const float Extents[3] = { MaxRange, pActor->Height, MaxRange }; //???pActor->Height or another height value?
	pNavQuery->findNearestPoly(Center.v, Extents, pNavFilter, &Ref, Pt.v);
	if (!Ref || vector3::SqDistance2D(Center, Pt) > SqMaxRange) return 0;

	if (SqMaxRange == 0.f)
	{
		Polys.Add(Ref);
		return 1;
	}

	const UPTR MAX_POLYS = 32; //!!!???to settings?!
	Polys.AllocateFixed(MAX_POLYS);
	int NearCount;
	if (!dtStatusSucceed(pNavQuery->findPolysAroundCircle(Ref, Pt.v, MaxRange, pNavFilter, Polys.Begin(), nullptr, nullptr, &NearCount, MAX_POLYS))) return 0;

	// Exclude polys laying entirely in MinRange. Since polys are convex, there is no chance
	// that some poly has all corners inside MinRange, but some part of area outside.
	if (SqMinRange > 0.f)
	{
		const dtNavMesh* pNavMesh = pNavQuery->getAttachedNavMesh();
		for (int i = 0; i < NearCount; )
		{
			dtPolyRef NearRef = Polys[i];
			const dtMeshTile* pTile;
			const dtPoly* pPoly;
			pNavMesh->getTileAndPolyByRefUnsafe(NearRef, &pTile, &pPoly);

			int j;
			for (j = 0; j < pPoly->vertCount; ++j)
				if (dtVdist2DSqr(pTile->verts + pPoly->verts[j] * 3, Center.v) > SqMinRange) break;

			if (j < pPoly->vertCount) ++i;
			else
			{
				--NearCount;
				if (i < NearCount) Polys[i] = Polys[NearCount];
			}
		}
	}

	Polys.Truncate(Polys.GetCount() - NearCount);
	return NearCount;
}
//---------------------------------------------------------------------

// Finds a valid position in range of [MinRange, MaxRange] from Center, that is the closest to the actor.
// Fails if there is no valid location in the specified zone, or if error occured.
// NB: this function can modify OutPos even if failed
bool CNavSystem::GetNearestValidLocation(const vector3& Center, float MinRange, float MaxRange, vector3& OutPos) const
{
	if (!pNavQuery || MinRange > MaxRange) FAIL;

	if (pActor->IsNavLocationValid() && pActor->IsAtPoint(Center, MinRange, MaxRange))
	{
		OutPos = pActor->Position;
		OK;
	}

	CArray<dtPolyRef> NearRefs;
	if (!GetValidPolys(Center, MinRange, MaxRange, NearRefs)) FAIL;

	return GetNearestValidLocation(NearRefs.Begin(), NearRefs.GetCount(), Center, MinRange, MaxRange, OutPos);
}
//---------------------------------------------------------------------

// Finds a valid position on specified nav region or in Range from its boundary, that is
// the closest to the actor. Fails if there is no valid location in the specified zone, or if error occured.
// NB: this function can modify OutPos even if failed
bool CNavSystem::GetNearestValidLocation(CStrID NavRegionID, float Range, vector3& OutPos) const
{
	//if (!pNavQuery) FAIL;

	////???AILevel::GetNavRegion instead?
	//CNavData* pNav = pActor->GetEntity()->GetLevel()->GetAI()->GetNavData(pActor->Radius);
	//if (!pNav) FAIL;

	//IPTR Idx = pNav->Regions.FindIndex(NavRegionID);
	//if (Idx == INVALID_INDEX) FAIL;
	//CNavRegion& Region = pNav->Regions.ValueAt(Idx);

	//return GetNearestValidLocation(Region.GetPtr(), Region.GetCount(), Range, OutPos);
	NOT_IMPLEMENTED;
	FAIL;
}
//---------------------------------------------------------------------

// Finds a valid position on specified nav poly set or in Range from its boundary, that is
// the closest to the actor. Fails if there is no valid location in the specified zone, or if error occured.
// NB: this function can modify OutPos even if failed
bool CNavSystem::GetNearestValidLocation(dtPolyRef* pPolys, int PolyCount, float Range, vector3& OutPos) const
{
	n_assert2_dbg(Range >= 0.f, "Use smaller regions or steer instead of using negative Range");

	if (!pNavQuery || !pPolys || PolyCount < 1 || Range < 0.f) FAIL;

	dtPolyRef NearestPoly = GetNearestPoly(pPolys, PolyCount, OutPos);
	n_assert_dbg(NearestPoly);

	// Optimization: we are already on the target poly, OutPos is our position, early exit
	if (NearestPoly == Corridor.getFirstPoly()) OK;

	if (Range == 0.f) OK;

	// Project the nearest point on poly towards the actor to obtain the really nearest one
	vector3 Diff = OutPos - pActor->Position;
	float SqDistance = Diff.SqLength2D();
	if (SqDistance <= Range * Range) OK;
	float Distance = n_sqrt(SqDistance);
	vector3 CandidatePos = pActor->Position + Diff * ((Distance - Range) / Distance);

	if (IsLocationValid(CandidatePos)) OutPos = CandidatePos;
	else
	{
		// Projected point is invalid, project back to find the first valid point between
		// the nearest and valid ones. It will be the closest valid point we search for.
		float t;
		if (!dtStatusSucceed(pNavQuery->raycast(NearestPoly, OutPos.v, CandidatePos.v, pNavFilter, &t, nullptr, nullptr, nullptr, 0))) FAIL;
		n_assert(t != FLT_MAX); // OutPos is valid, CandidatePos is not. Intersection must exist.
		OutPos += (CandidatePos - OutPos) * t;
	}

	OK;
}
//---------------------------------------------------------------------

// This version allows to query in-range position from specified set of polys. This one can be used
// for non-moving targets, where poly set could be collected once.
// NB: this function can modify OutPos even if failed
bool CNavSystem::GetNearestValidLocation(dtPolyRef* pPolys, int PolyCount, const vector3& Center, float MinRange, float MaxRange, vector3& OutPos) const
{
	if (!pNavQuery || !pPolys || PolyCount < 1 || MinRange > MaxRange) FAIL;

	const float SqMinRange = MinRange * MinRange;
	const float SqMaxRange = MaxRange * MaxRange;

	dtPolyRef NearestPoly = GetNearestPoly(pPolys, PolyCount, OutPos);
	n_assert_dbg(NearestPoly);

	// If OutPos is in [MinRange, MaxRange], return it, else select closest range

	float SqRange = vector3::SqDistance2D(OutPos, Center);
	if (SqRange > SqMaxRange) SqRange = SqMaxRange;
	else if (SqRange < SqMinRange) SqRange = SqMinRange;
	else OK;

	// Use segment-circle intersection to project OutPos onto the closest range.
	// Segment is defined by OutPos, which is an actor's position projected to the nearest poly,
	// and by ProjCenter, which is a Center projected to the nearest poly, so the segment is the
	// shortest path from an actor to the Center along the nearest poly surface.

	vector3 ProjCenter;
	bool IsOverPoly;
	pNavQuery->closestPointOnPoly(NearestPoly, Center.v, ProjCenter.v, &IsOverPoly);

	vector3 SegDir = OutPos - ProjCenter;
	vector3 RelProjCenter = ProjCenter - Center;
	float A = SegDir.SqLength2D();
	float B = 2.f * RelProjCenter.Dot2D(SegDir);
	float C = RelProjCenter.SqLength2D() - SqRange;
	float t1, t2;
	UPTR RootCount = Math::SolveQuadraticEquation(A, B, C, &t1, &t2);
	n_assert2(RootCount, "No solution found for the closest point though theoretically there must be one");
	float t = (RootCount == 1 || t1 > t2) ? t1 : t2; // Greater t is closer to actor, which is desired
	n_assert_dbg(t >= 0.f && t <= 1.f);
	OutPos = ProjCenter + SegDir * t;

	OK;
}
//---------------------------------------------------------------------

bool CNavSystem::GetRandomValidLocation(float Range, vector3& Location)
{
	if (pActor->NavState == AINav_Invalid) FAIL;

	if (!pNavQuery) FAIL;

	//!!!Need to clamp to radius!
	dtPolyRef Ref;
	return dtStatusSucceed(pNavQuery->findRandomPointAroundCircle(Corridor.getFirstPoly(), pActor->Position.v, Range,
		pNavFilter, n_rand, &Ref, Location.v));
}
//---------------------------------------------------------------------
*/
