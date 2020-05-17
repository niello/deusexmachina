#include <Game/GameLevel.h>
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <Game/ECS/Components/SceneComponent.h>
#include <AI/Navigation/NavAgentComponent.h>
#include <AI/Navigation/PathRequestQueue.h>
#include <AI/Navigation/TraversalAction.h>
#include <DetourCommon.h>
#include <array>

namespace DEM::AI
{

//???set height limit to check? agent height is good, must distinguish between different floors.
//otherwise false equalities may happen!
constexpr float DEFAULT_EPSILON = 1.0f / (16384.0f * 16384.0f);
static inline bool dtVequal2D(const float* p0, const float* p1, float thr = DEFAULT_EPSILON)
{
	return dtVdist2DSqr(p0, p1) < thr;
}
//---------------------------------------------------------------------

// Returns whether an agent can continue to perform the current navigation task
static bool UpdatePosition(const vector3& Position, CNavAgentComponent& Agent)
{
	const float RecoveryRadius = std::min(Agent.Radius * 2.f, 20.f);
	const float SqRecoveryThreshold = Agent.Radius * Agent.Radius * 0.01f; // thr = 0.1 * agent.R
	const auto* pNavFilter = Agent.Settings->GetQueryFilter();

	if (Agent.Mode != ENavigationMode::Offmesh &&
		Agent.State != ENavigationState::Idle &&
		!dtVequal2D(Agent.Corridor.getPos(), Position.v))
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
			else if (SqDeviationFromCorridor <= RecoveryRadius)
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

		// Make sure the first polygon is valid, but leave other valid
		// polygons in the path so that replanner can adjust the path better.
		Agent.Corridor.fixPathStart(NearestRef, NearestPos);
		Agent.State = ENavigationState::Requested;
	}

	return true;
}
//---------------------------------------------------------------------

// Returns whether an agent can continue to perform the current navigation task
static bool UpdateDestination(Navigate& Action, CNavAgentComponent& Agent)
{
	//!!!FIXME: setting, per Navigate action or per entity!
	constexpr float MaxTargetOffset = 0.5f;
	constexpr float MaxTargetOffsetSq = MaxTargetOffset * MaxTargetOffset;

	const auto* pNavFilter = Agent.Settings->GetQueryFilter();

	if (Agent.State != ENavigationState::Idle)
	{
		if (dtVequal2D(Agent.TargetPos.v, Action._Destination.v) &&
			n_fabs(Agent.TargetPos.y - Action._Destination.y) < Agent.Height)
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
				if (OffsetSq < DEFAULT_EPSILON ||
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

		Agent.AsyncTaskID = PathQueue.Request(Agent.Corridor.getLastPoly(), Agent.TargetRef, Agent.Corridor.getTarget(), Agent.TargetPos.v, Agent.pNavQuery, pNavFilter);
		if (Agent.AsyncTaskID)
			Agent.State = ENavigationState::Planning;
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

// TODO: use DT_STRAIGHTPATH_ALL_CROSSINGS for controlled area, where each poly can have personal controller and action
static bool GenerateTraversalAction(CNavAgentComponent& Agent, Game::CActionQueueComponent& Queue,
	const Navigate& NavAction, vector3& OutDest)
{
	const float* pCurrPos = Agent.Corridor.getPos();
	const auto* pCurrPath = Agent.Corridor.getPath();
	dtStraightPathContext Ctx;
	if (dtStatusFailed(Agent.pNavQuery->initStraightPathSearch(
			pCurrPos, Agent.Corridor.getTarget(), pCurrPath, Agent.Corridor.getPathCount(), Ctx)))
		return false;

	vector3 NextDest;
	unsigned char Flags;
	unsigned char AreaType;
	dtPolyRef PolyRef;
	dtStatus Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, NextDest.v, &Flags, &AreaType, &PolyRef, DT_STRAIGHTPATH_AREA_CROSSINGS);
	if (dtStatusFailed(Status)) return false;

	// If no action available, give an agent a chance to trigger an offmesh connection
	//???!!!cache current smart object until poly changes?
	Game::HEntity SmartObject;
	auto pAction = Agent.Settings->FindAction(Agent, Agent.CurrAreaType, pCurrPath[0], &SmartObject);
	if (!pAction && !(Flags & DT_STRAIGHTPATH_OFFMESH_CONNECTION)) return false;

	// Advance through path points until the traversal action is generated
	vector3 Dest;
	do
	{
		// Elongate path edge
		Dest = NextDest;

		// That was the last edge, finish
		if (!dtStatusInProgress(Status)) break;

		const float DistanceSq = dtVdist2DSqr(pCurrPos, Dest.v);

		// Check entering an offmesh connection
		if (Flags & DT_STRAIGHTPATH_OFFMESH_CONNECTION)
		{
			const float OffmeshTriggerRadius = Agent.Radius * 2.f;
			if (DistanceSq < OffmeshTriggerRadius * OffmeshTriggerRadius)
			{
				auto pOffmeshAction = Agent.Settings->FindAction(Agent, AreaType, PolyRef, &SmartObject);
				if (!pOffmeshAction) break;

				// Adjust the path over the off-mesh connection.
				// Path validity check will ensure that bad/blocked connections will be replanned.
				// NB: for offmesh actions Dest is offmesh start and NextDest is offmesh end
				dtPolyRef OffmeshRefs[2];
				if (Agent.Corridor.moveOverOffmeshConnection(PolyRef, OffmeshRefs, Dest.v, NextDest.v, Agent.pNavQuery))
				{
					pAction = pOffmeshAction;
					Agent.Mode = ENavigationMode::Offmesh;
				}
			}

			//???break always or only at different action? what if offmesh offered steering?
			//!!!if !pAction, must break!
			break;
		}

		// Close enough to the next path point, assume already traversed
		if (pAction->CanSkipPathPoint(DistanceSq))
		{
			Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, NextDest.v, &Flags, &AreaType, &PolyRef, DT_STRAIGHTPATH_AREA_CROSSINGS);
			if (dtStatusFailed(Status)) return false;
			continue;
		}

		//???pack into pAction->IsContinuableBy/IsSame(pNextAction)?
		// The next path edge requires different traversal action, finish
		Game::HEntity NextSmartObject;
		auto pNextAction = Agent.Settings->FindAction(Agent, AreaType, PolyRef, &NextSmartObject);
		if (!pNextAction || pAction->GetRTTI() != pNextAction->GetRTTI() || SmartObject != NextSmartObject) break;

		// Get the next path edge end point
		Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, NextDest.v, &Flags, &AreaType, &PolyRef, DT_STRAIGHTPATH_AREA_CROSSINGS);
		if (dtStatusFailed(Status)) break;

		// If next edge changes direction, use its end position for smooth turning and finish
		constexpr float COS_SMALL_ANGLE_SQ = 0.99999f;
		const vector2 ToCurr(Dest.x - pCurrPos[0], Dest.z - pCurrPos[2]);
		const vector2 ToNext(NextDest.x - pCurrPos[0], NextDest.z - pCurrPos[2]);
		const float Dot = ToNext.dot(ToCurr);
		if (Dot * Dot < ToNext.SqLength() * ToCurr.SqLength() * COS_SMALL_ANGLE_SQ)
			break;
	}
	while (true);

	if (!pAction) return false;

	// TODO: NEW ACTIONS
	// create/get action instance by type as sub-action of Navigate
	// set Dest & NextDest
	// if need to update distance after dest, calc and set to new action instance distance to the next action / to final target

	// Push sub-action on top of the navigation action, updating or clearing previous one
	OutDest = Dest;
	auto Result = pAction->PushSubAction(Queue, NavAction, Dest, NextDest, SmartObject);
	if (Result & CTraversalAction::Failure) return false;

	// Some controllers require additional distance from current to final destination
	if (Result & CTraversalAction::NeedDistanceToTarget)
	{
		float Distance = vector3::Distance(Dest, NextDest);
		vector3 PrevPathPoint = NextDest;
		while (dtStatusInProgress(Status))
		{
			vector3 PathPoint;
			Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, PathPoint.v, &Flags, &AreaType, &PolyRef, DT_STRAIGHTPATH_AREA_CROSSINGS);
			if (dtStatusFailed(Status)) break;
			Distance += vector3::Distance(PrevPathPoint, PathPoint);
			PrevPathPoint = PathPoint;
		}

		pAction->SetDistanceToTarget(Queue, NavAction, Distance);
	}

	return true;
}
//---------------------------------------------------------------------

static void ResetNavigation(CNavAgentComponent& Agent, Game::CActionQueueComponent& Queue, DEM::Game::EActionStatus Result)
{
	while (auto pNavigateAction = Queue.FindActive<Navigate>())
		Queue.RemoveAction(*pNavigateAction, Result);

	if (auto CurrPoly = Agent.Corridor.getFirstPoly())
		Agent.Corridor.reset(CurrPoly, Agent.Corridor.getPos());

	Agent.State = ENavigationState::Idle;
	if (Agent.Mode == ENavigationMode::Offmesh)
		Agent.Mode = ENavigationMode::Recovery;
}
//---------------------------------------------------------------------

void ProcessNavigation(DEM::Game::CGameWorld& World, float dt, ::AI::CPathRequestQueue& PathQueue, bool NewFrame)
{
	World.ForEachEntityWith<CNavAgentComponent, DEM::Game::CActionQueueComponent, const DEM::Game::CSceneComponent>(
		[dt, &PathQueue, NewFrame](auto EntityID, auto& Entity,
			CNavAgentComponent& Agent,
			DEM::Game::CActionQueueComponent* pQueue,
			const DEM::Game::CSceneComponent* pSceneComponent)
	{
		if (!pSceneComponent->RootNode || !Agent.pNavQuery || !Agent.Settings) return;

		// Update navigation status from the curent agent position
		if (!UpdatePosition(pSceneComponent->RootNode->GetWorldPosition(), Agent))
		{
			ResetNavigation(Agent, *pQueue, DEM::Game::EActionStatus::Failed);
			return;
		}

		if (auto pNavigateAction = pQueue->FindActive<Navigate>())
		{
			// If sub-action failed, fail navigation
			if (pQueue->GetStatus() == DEM::Game::EActionStatus::Failed)
			{
				ResetNavigation(Agent, *pQueue, DEM::Game::EActionStatus::Failed);
				return;
			}

			// Multiple physics frames can be processed inside one logic frame. Target remains the
			// same during the logic frame but might be reached. Character position is already processed.
			// So if there is a sub-action, let's just continue executing it, without unnecessary update.
			if (!NewFrame && pQueue->GetActiveStackTop() != pNavigateAction) return;

			// NB: in Detour replan time increases only when on navmesh (not offmesh, not invalid)
			Agent.ReplanTime += dt;

			// Process target location changes and validity
			if (!UpdateDestination(*pNavigateAction, Agent))
			{
				ResetNavigation(Agent, *pQueue, DEM::Game::EActionStatus::Failed);
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
					ResetNavigation(Agent, *pQueue, DEM::Game::EActionStatus::Failed);
					return;
				}
			}

			// Generate sub-action for path following
			if (Agent.Mode == ENavigationMode::Recovery)
			{
				// Try to return to the navmesh by the shortest path
				const vector3 ValidPos = Agent.Corridor.getPos();
				auto pAction = Agent.Settings->FindAction(Agent, 0, 0, nullptr);
				if (!pAction || (pAction->PushSubAction(*pQueue, *pNavigateAction, ValidPos, ValidPos, {}) & CTraversalAction::Failure))
					ResetNavigation(Agent, *pQueue, DEM::Game::EActionStatus::Failed);
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

				//!!!!!!!!!!!!!!!!!!!!!!!!!
				//!!!!!!check if THE LAST action is finished successfully, then remove navigation task with success!

				vector3 ActionDest;
				if (GenerateTraversalAction(Agent, *pQueue, *pNavigateAction, ActionDest))
				{
					// Use our traversal target to optimize the path, because we know that there is a straight way to it
					if (OptimizePath && Agent.Mode == ENavigationMode::Surface)
						Agent.Corridor.optimizePathVisibility(ActionDest.v, 30.f * Agent.Radius, Agent.pNavQuery, pNavFilter);
				}
				else
				{
					ResetNavigation(Agent, *pQueue, DEM::Game::EActionStatus::Failed);
				}
			}
		}
		else if (Agent.State != ENavigationState::Idle)
		{
			ResetNavigation(Agent, *pQueue, DEM::Game::EActionStatus::Cancelled);
		}
	});
}
//---------------------------------------------------------------------

}
