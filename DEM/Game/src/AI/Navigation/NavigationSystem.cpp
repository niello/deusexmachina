#include <Game/ECS/GameWorld.h>
#include <Game/GameLevel.h>
#include <Game/GameSession.h>
#include <AI/Navigation/NavAgentComponent.h>
#include <AI/Navigation/NavControllerComponent.h>
#include <AI/Navigation/PathRequestQueue.h>
#include <AI/Navigation/TraversalAction.h>
#include <AI/Navigation/NavMeshDebugDraw.h>
#include <AI/Navigation/NavMesh.h>
#include <AI/Movement/SteerAction.h> // FIXME: only for Steer::SqLinearTolerance, can write better?
#include <Physics/CharacterControllerComponent.h>
#include <Physics/CollisionShape.h>
#include <Physics/BulletConv.h>
#include <Debug/DebugDraw.h>
#include <DetourCommon.h>
#include <DetourDebugDraw.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <array>

namespace DEM::AI
{
constexpr float EQUALITY_THRESHOLD_SQ = 1.0f / (16384.0f * 16384.0f);

// Returns whether an agent can continue to perform the current navigation task
static bool UpdatePosition(const rtm::vector4f& Position, CNavAgentComponent& Agent)
{
	const auto* pNavFilter = Agent.Settings->GetQueryFilter();

	// Agent is traversing an offmesh connection.
	// If current offmesh connection is invalid, fail navigation task. Otherwise ignore
	// the poly under feet. We may temporarily leave the surface while traversing offmesh.
	if (Agent.OffmeshRef) return Agent.pNavQuery->isValidPolyRef(Agent.OffmeshRef, pNavFilter);

	// When already recovering, reduce threshold 10 times to avoid jitter on the poly border
	const float SqRecoveryThreshold = Agent.Radius * Agent.Radius *
		((Agent.Mode == ENavigationMode::Recovery) ? 0.0001f : 0.01f);
	const float RecoveryRadius = std::min(Agent.Radius * 2.f, 20.f);

	const auto PositionRaw = Math::FromSIMD3(Position);
	if (Agent.State == ENavigationState::Idle ||
		(Agent.Mode == ENavigationMode::Surface &&
			InRange(Agent.Corridor.getPos(), PositionRaw.v, Agent.Height, EQUALITY_THRESHOLD_SQ)))
	{
		// Agent is idle, check the poly under feet validity
		if (Agent.pNavQuery->isValidPolyRef(Agent.Corridor.getFirstPoly(), pNavFilter)) return true;
	}
	else
	{
		// Agent is moving along the surface or recovering to it
		// NB: when moving in Recovery mode, it is possible that Agent.Corridor.getPos() == Position

		const auto PrevPoly = Agent.Corridor.getFirstPoly();

		// Agent moves along the navmesh surface or recovers to it, adjust the corridor
		if (Agent.Corridor.movePosition(PositionRaw.v, Agent.pNavQuery, pNavFilter))
		{
			const float SqDeviationFromCorridor = dtVdist2DSqr(PositionRaw.v, Agent.Corridor.getPos());
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

	// Our current poly is unknown or invalid, find the nearest valid one
	const float RecoveryExtents[3] = { RecoveryRadius, Agent.Height, RecoveryRadius };
	dtPolyRef NearestRef = 0;
	float NearestPos[3];
	Agent.pNavQuery->findNearestPoly(PositionRaw.v, RecoveryExtents, pNavFilter, &NearestRef, NearestPos);

	if (!NearestRef)
	{
		// No poly found in a recovery radius, agent can't recover and needs external position change
		Agent.Corridor.reset(0, PositionRaw.v);
		return false;
	}

	// Check if our new location is far enough from the navmesh to enable recovery mode
	if (dtVdist2DSqr(PositionRaw.v, NearestPos) > SqRecoveryThreshold)
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
static bool UpdateDestination(const rtm::vector4f& Dest, CNavAgentComponent& Agent, ::AI::CPathRequestQueue& PathQueue, bool& OutDestChanged)
{
	//!!!FIXME: setting, per Navigate action or per entity!
	constexpr float MaxTargetOffset = 0.5f;
	constexpr float MaxTargetOffsetSq = MaxTargetOffset * MaxTargetOffset;

	const auto* pNavFilter = Agent.Settings->GetQueryFilter();

	const auto DestRaw = Math::FromSIMD3(Dest);

	if (Agent.State != ENavigationState::Idle)
	{
		OutDestChanged = !InRange(Agent.TargetPos.v, DestRaw.v, Agent.Height, EQUALITY_THRESHOLD_SQ);
		if (!OutDestChanged)
		{
			// Target remains static but we must check its poly validity anyway
			if (Agent.pNavQuery->isValidPolyRef(Agent.TargetRef, pNavFilter)) return true;
		}
		else if (Agent.State == ENavigationState::Following)
		{
			// Planning finished, can adjust moving target in the corridor
			if (Agent.Corridor.moveTargetPosition(DestRaw.v, Agent.pNavQuery, pNavFilter))
			{
				// Check if target matches the corridor or moved not too far from it into impassable zone.
				// Otherwise target is considered teleported and may require replanning (see below).
				const auto OffsetSq = dtVdist2DSqr(DestRaw.v, Agent.Corridor.getTarget());
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
	else OutDestChanged = true;

	// Target poly is unknown or invalid, find the nearest valid one
	const float TargetExtents[3] = { MaxTargetOffset, Agent.Height, MaxTargetOffset };
	Agent.pNavQuery->findNearestPoly(DestRaw.v, TargetExtents, pNavFilter, &Agent.TargetRef, Agent.TargetPos.v);

	// If no poly found in a target radius, navigation task is failed
	if (!Agent.TargetRef || dtVdist2DSqr(DestRaw.v, Agent.TargetPos.v) > MaxTargetOffsetSq)
		return false;

	// If target is in the current corridor, can avoid replanning
	if (Agent.State != ENavigationState::Idle)
	{
		bool OffmeshActual = false;
		for (int i = 0; i < Agent.Corridor.getPathCount(); ++i)
		{
			if (Agent.Corridor.getPath()[i] == Agent.OffmeshRef)
				OffmeshActual = true;

			if (Agent.Corridor.getPath()[i] == Agent.TargetRef)
			{
				Agent.Corridor.shrink(Agent.TargetPos.v, i + 1);
				if (Agent.AsyncTaskID)
				{
					PathQueue.CancelRequest(Agent.AsyncTaskID);
					Agent.AsyncTaskID = 0;
				}
				Agent.State = ENavigationState::Following;

				// Offmesh traversal can't be interrupted, but preparation can
				if (Agent.Mode == ENavigationMode::Surface && !OffmeshActual)
					Agent.OffmeshRef = 0;

				return true;
			}
		}
	}

	Agent.State = ENavigationState::Requested;

	// Offmesh traversal can't be interrupted, but preparation can
	if (Agent.Mode == ENavigationMode::Surface)
		Agent.OffmeshRef = 0;

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
	Agent.pNavQuery->updateSlicedFindPath(20, nullptr);

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
		Agent.PathOptimizationTime = 0.f;
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
	Agent.PathOptimizationTime = 0.f;
	return true;
}
//---------------------------------------------------------------------

static void ResetNavigation(CNavAgentComponent& Agent, ::AI::CPathRequestQueue& PathQueue,
	Game::CActionQueueComponent& Queue, Game::HAction NavAction, AI::ECommandStatus Result)
{
	if (Agent.AsyncTaskID)
	{
		PathQueue.CancelRequest(Agent.AsyncTaskID);
		Agent.AsyncTaskID = 0;
	}

	if (auto CurrPoly = Agent.Corridor.getFirstPoly())
		Agent.Corridor.reset(CurrPoly, Agent.Corridor.getPos());

	Agent.State = ENavigationState::Idle;
	Agent.OffmeshRef = 0;
	if (Agent.Mode == ENavigationMode::Offmesh)
	{
		Agent.Mode = ENavigationMode::Recovery;
		Agent.CurrAreaType = 0;
	}

	Queue.SetStatus(NavAction, Result);
}
//---------------------------------------------------------------------

//!!!FIXME: if character stands exactly in the offmesh connection start, findNextStraightPathPoint will return
//other side of the offmesh connection. Offmesh will not be triggered, its action will be ignored.
static CTraversalAction* FindTraversalAction(Game::CGameWorld& World, CNavAgentComponent& Agent, Game::CActionQueueComponent& Queue,
	Game::HAction NavAction, const rtm::vector4f& Pos, bool OptimizePath, Game::HEntity& OutController)
{
	if (Agent.Mode == ENavigationMode::Recovery)
		return Agent.Settings->FindAction(World, Agent, 0, 0, nullptr);

	CTraversalAction* pAction = nullptr;

	if (Agent.Mode == ENavigationMode::Surface)
	{
		unsigned char AreaType;

		if (Agent.OffmeshRef)
		{
			//???FIXME: where to clear OffmeshRef? probably remains filled when must not be, and breaks logic!
			//!!!must never assert! If path is failed or replanned, OffmeshRef must be cleared!
			n_assert(Agent.State == ENavigationState::Following);

			// Already preparing to traverse offmesh connection
			Agent.NavMap->GetDetourNavMesh()->getPolyArea(Agent.OffmeshRef, &AreaType);
			pAction = Agent.Settings->FindAction(World, Agent, AreaType, Agent.OffmeshRef, &OutController);
		}
		else if (Agent.State == ENavigationState::Following)
		{
			// FIXME: only if necessary events happened? like offsetting from expected position.
			// The more inaccurate the agent movement, the more beneficial this function becomes. (c) Docs
			// The same for optimizePathVisibility (used below).
			if (OptimizePath)
				Agent.Corridor.optimizePathTopology(Agent.pNavQuery, Agent.Settings->GetQueryFilter());

			// Prepare straight path context for the next corner search
			dtStraightPathContext Ctx;
			if (dtStatusFailed(Agent.pNavQuery->initStraightPathSearch(
				Agent.Corridor.getPos(), Agent.Corridor.getTarget(), Agent.Corridor.getPath(), Agent.Corridor.getPathCount(), Ctx)))
			{
				return nullptr;
			}

			// Find the next corner or offmesh start, ignore area and poly changes
			vector3 NextCorner;
			unsigned char Flags;
			dtPolyRef PolyRef;
			if (dtStatusFailed(Agent.pNavQuery->findNextStraightPathPoint(Ctx, NextCorner.v, &Flags, &AreaType, &PolyRef, 0)))
				return nullptr;

			// The next corner is an offmesh connection, try to trigger it
			if (Flags & DT_STRAIGHTPATH_OFFMESH_CONNECTION)
			{
				// Check if an agent can traverse this connection
				if (auto pOffmeshAction = Agent.Settings->FindAction(World, Agent, AreaType, PolyRef, &OutController))
				{
					const auto* pOffmesh = Agent.NavMap->GetDetourNavMesh()->getOffMeshConnectionByRef(PolyRef);
					if (!pOffmesh) return nullptr;

					// Check if we are in a trigger range
					if (InRange(Math::FromSIMD3(Pos).v, NextCorner.v, Agent.Height, pOffmeshAction->GetSqTriggerRadius(Agent.Radius, pOffmesh->rad)))
					{
						Agent.OffmeshRef = PolyRef;
						pAction = pOffmeshAction;
					}
				}
			}

			// If no offmesh triggered, optimize path visibility along the [corridor pos -> NextCorner] straight line
			if (OptimizePath && !pAction)
				Agent.Corridor.optimizePathVisibility(NextCorner.v, 30.f * Agent.Radius, Agent.pNavQuery, Agent.Settings->GetQueryFilter());
		}

		// If no offmesh triggered, traverse surface
		if (!pAction) return Agent.Settings->FindAction(World, Agent, Agent.CurrAreaType, Agent.Corridor.getFirstPoly(), &OutController);

		// Offmesh connection is triggered. Start traversal if preconditions are met.
		if (pAction->CanStartTraversingOffmesh(World, Agent, OutController, Pos))
		{
			dtPolyRef OffmeshRefs[2];
			vector3 Start, End;
			if (!Agent.Corridor.moveOverOffmeshConnection(Agent.OffmeshRef, OffmeshRefs, Start.v, End.v, Agent.pNavQuery))
				return nullptr;

			Agent.Mode = ENavigationMode::Offmesh;
			Agent.CurrAreaType = AreaType;
		}
	}

	// NB: Offmesh mode could be set during the Surface mode processing, see above
	if (Agent.Mode == ENavigationMode::Offmesh)
	{
		// Offmesh action could already have been found in Surface mode
		if (!pAction)
		{
			pAction = Agent.Settings->FindAction(World, Agent, Agent.CurrAreaType, Agent.OffmeshRef, &OutController);
			if (!pAction) return nullptr;
		}

		// End offmesh traversal if preconditions are met, return Surface action immediately
		if (pAction->CanEndTraversingOffmesh(World, Agent, OutController, Pos))
		{
			Agent.Mode = ENavigationMode::Surface;
			Agent.OffmeshRef = 0;
			Agent.pNavQuery->getAttachedNavMesh()->getPolyArea(Agent.Corridor.getFirstPoly(), &Agent.CurrAreaType);

			if (!UpdatePosition(Pos, Agent)) return nullptr;

			return Agent.Settings->FindAction(World, Agent, Agent.CurrAreaType, Agent.Corridor.getFirstPoly(), &OutController);
		}
	}

	return pAction;
}
//---------------------------------------------------------------------

static bool HasArrived(CNavAgentComponent& Agent, const rtm::vector4f& Pos, float SqArrivalRadius, bool Unobstructed)
{
	if (Agent.Mode == ENavigationMode::Offmesh) return false;

	const rtm::vector4f ToDest = rtm::vector_sub(Pos, rtm::vector_load3(Agent.Corridor.getTarget()));

	if (Math::vector_length_squared_xz(ToDest) > std::max(SqArrivalRadius, Steer::SqLinearTolerance)) return false;

	// NB: in unobstructed case we allow to break this height constraint, because straight path check is better
	if (!Unobstructed) return std::abs(rtm::vector_get_y(ToDest)) < Agent.Height;

	dtStraightPathContext Ctx;
	if (dtStatusFailed(Agent.pNavQuery->initStraightPathSearch(
		Agent.Corridor.getPos(), Agent.Corridor.getTarget(), Agent.Corridor.getPath(), Agent.Corridor.getPathCount(), Ctx)))
	{
		return false;
	}

	// Succeeds when reached the target, otherwise returns "in progress"
	return dtStatusSucceed(Agent.pNavQuery->findNextStraightPathPoint(Ctx, nullptr, nullptr, nullptr, nullptr, 0));
}
//---------------------------------------------------------------------

void InitNavigationAgents(Game::CGameWorld& World, Game::CGameLevel& Level, Resources::CResourceManager& ResMgr)
{
	World.ForEachComponent<CNavAgentComponent>([&Level, &ResMgr](auto EntityID, CNavAgentComponent& NavAgent)
	{
		auto Rsrc = ResMgr.RegisterResource<CNavAgentSettings>(NavAgent.SettingsID.CStr());
		NavAgent.Settings = Rsrc->ValidateObject<CNavAgentSettings>();

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

// NB: PathQueue is updated here, after components
void ProcessNavigation(DEM::Game::CGameSession& Session, float dt, ::AI::CPathRequestQueue& PathQueue, bool NewFrame)
{
	auto pWorld = Session.FindFeature<DEM::Game::CGameWorld>();
	if (!pWorld) return;

	auto& World = *pWorld;

	World.ForEachEntityWith<CNavAgentComponent, Game::CActionQueueComponent, const Game::CCharacterControllerComponent>(
		[dt, &Session, &World, &PathQueue, NewFrame](auto EntityID, auto& Entity,
			CNavAgentComponent& Agent,
			Game::CActionQueueComponent& Queue,
			const Game::CCharacterControllerComponent& Character)
	{
		if (!Character.RigidBody || !Agent.pNavQuery || !Agent.Settings) return;

		const auto PrevState = Agent.State;
		const auto PrevMode = Agent.Mode;

		auto NavigateAction = Queue.FindCurrent<Navigate>();

		// Access real physical transform, not an interpolated motion state
		const auto& Offset = Character.RigidBody->GetCollisionShape()->GetOffset();
		const rtm::vector4f Pos = Math::FromBullet(Character.RigidBody->GetBtBody()->getWorldTransform() * Math::ToBullet3(rtm::vector_neg(Offset)));

		// Update navigation status from the current agent position
		if (!UpdatePosition(Pos, Agent))
		{
			ResetNavigation(Agent, PathQueue, Queue, NavigateAction, AI::ECommandStatus::Failed);
			return;
		}

		if (!NavigateAction)
		{
			if (Agent.State != ENavigationState::Idle)
				ResetNavigation(Agent, PathQueue, Queue, NavigateAction, AI::ECommandStatus::Cancelled);
			return;
		}

		// Action could fail but remain in a queue
		// FIXME: new action execution system must address this
		const auto ActionStatus = Queue.GetStatus(NavigateAction);
		if (ActionStatus == AI::ECommandStatus::Failed || ActionStatus == AI::ECommandStatus::Cancelled)
			return;

		// Sub-action termination may lead to a navigation action termination
		const auto SubActionStatus = Queue.GetStatus(Queue.GetChild(NavigateAction));
		if (SubActionStatus == AI::ECommandStatus::Failed ||
			SubActionStatus == AI::ECommandStatus::Cancelled ||
			(SubActionStatus == AI::ECommandStatus::Succeeded && HasArrived(Agent, Pos, 0.f, true)))
		{
			ResetNavigation(Agent, PathQueue, Queue, NavigateAction, SubActionStatus);
			return;
		}

		const bool HasActiveSubAction = (SubActionStatus == AI::ECommandStatus::Running);

		// Multiple physics frames can be processed inside one logic frame. Target remains the same
		// during the logic frame but might be reached by physics in the middle of it. So if there
		// is an active sub-action, let's just continue executing it, without unnecessary update.
		if (!NewFrame && HasActiveSubAction && PrevMode == Agent.Mode && Agent.State == ENavigationState::Following) return;

		// Process target location changes and validity
		bool DestChanged = true;
		if (!UpdateDestination(NavigateAction.As<Navigate>()->_Destination, Agent, PathQueue, DestChanged))
		{
			ResetNavigation(Agent, PathQueue, Queue, NavigateAction, AI::ECommandStatus::Failed);
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
				ResetNavigation(Agent, PathQueue, Queue, NavigateAction, AI::ECommandStatus::Failed);
				return;
			}
		}

		// Try to continue execution of an active sub-action
		bool OptimizePath = false;
		if (Agent.OffmeshRef)
		{
			// Offmesh connection properties can't change and therefore sub-action can't become inactual during the traversal
			if (HasActiveSubAction && PrevState == Agent.State && (!DestChanged || Agent.Mode == ENavigationMode::Offmesh)) return;
		}
		else if (Agent.Mode == ENavigationMode::Surface)
		{
			// When path planning is done, we periodically optimize it and can replan if necessary
			if (Agent.State == ENavigationState::Following)
			{
				Agent.ReplanTime += dt;
				Agent.PathOptimizationTime += dt;

				constexpr float OPT_TIME_THR_SEC = 2.5f;
				OptimizePath = (Agent.PathOptimizationTime >= OPT_TIME_THR_SEC);
				if (OptimizePath) Agent.PathOptimizationTime = 0.f;
			}

			// Continue executing active sub-action while it is actual
			if (!DestChanged && !OptimizePath && HasActiveSubAction && PrevState == Agent.State) return;
		}

		// Generate sub-action for path following
		Game::HEntity Controller;
		auto pAction = FindTraversalAction(World, Agent, Queue, NavigateAction, Pos, OptimizePath, Controller);
		if (!pAction || !pAction->GenerateAction(Session, Agent, EntityID, Controller, Queue, NavigateAction, Pos))
			ResetNavigation(Agent, PathQueue, Queue, NavigateAction, AI::ECommandStatus::Failed);
	});

	// Execute async path requests
	// FIXME: use multithreading!
	PathQueue.Update(100);
}
//---------------------------------------------------------------------

void RenderDebugNavigation(Game::CGameWorld& World, Debug::CDebugDraw& DebugDraw)
{
	World.ForEachEntityWith<const CNavAgentComponent, const Game::CCharacterControllerComponent>(
		[&DebugDraw](auto EntityID, auto& Entity,
			const CNavAgentComponent& Agent,
			const Game::CCharacterControllerComponent& Character)
	{
		if (!Character.RigidBody || !Agent.pNavQuery) return;

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
				const auto& Offset = Character.RigidBody->GetCollisionShape()->GetOffset();
				rtm::vector4f From = Math::FromBullet(Character.RigidBody->GetBtBody()->getWorldTransform() * Math::ToBullet3(rtm::vector_neg(Offset)));
				rtm::vector4f To;
				dtStatus Status;
				U8 AreaType = Agent.CurrAreaType;
				do
				{
					const int Options = Agent.Settings->IsAreaControllable(AreaType) ? DT_STRAIGHTPATH_ALL_CROSSINGS : DT_STRAIGHTPATH_AREA_CROSSINGS;
					float ToRaw[3];
					Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, ToRaw, nullptr, &AreaType, nullptr, Options);
					if (dtStatusFailed(Status)) break;

					To = rtm::vector_load3(ToRaw);
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

void DestroyNavigationAgent(Game::CGameWorld& World, ::AI::CPathRequestQueue& PathQueue, Game::HEntity EntityID, CNavAgentComponent& Agent)
{
	// Cancel active navigation tasks
	if (auto pQueue = World.FindComponent<Game::CActionQueueComponent>(EntityID))
	{
		Game::HAction TopNavAction;
		while (auto NavAction = pQueue->FindCurrent<Navigate>(TopNavAction))
			TopNavAction = NavAction;

		if (TopNavAction && pQueue->GetStatus(TopNavAction) == AI::ECommandStatus::Running)
			pQueue->SetStatus(TopNavAction, AI::ECommandStatus::Cancelled);
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
}
//---------------------------------------------------------------------

void DestroyNavigation(Game::CGameWorld& World, ::AI::CPathRequestQueue& PathQueue)
{
	World.RemoveAllComponents<CNavAgentComponent>();
}
//---------------------------------------------------------------------

// TODO: unregister!!!
void RegisterNavigationControllers(Game::CGameWorld& World)
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
