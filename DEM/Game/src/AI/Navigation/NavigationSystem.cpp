#include "NavSystem.h"
#include <Game/GameLevel.h>
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <Game/ECS/Components/SceneComponent.h>
#include <AI/Navigation/NavAgentComponent.h>
#include <AI/Navigation/PathRequestQueue.h>
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

static void UpdatePosition(const vector3& Position, CNavAgentComponent& Agent)
{
	const bool PositionChanged = !dtVequal2D(Agent.Corridor.getPos(), Position.v);

	if (PositionChanged && Agent.Mode != ENavigationMode::Offmesh && Agent.State != ENavigationState::Idle)
	{
		const auto PrevPoly = Agent.Corridor.getFirstPoly();

		// Agent moves along the navmesh surface or recovers to it, adjust the corridor
		if (Agent.Corridor.movePosition(Position.v, Agent.pNavQuery, Agent.pNavFilter))
		{
			if (dtVequal2D(Position.v, Agent.Corridor.getPos()))
			{
				// TODO: finish recovery, if was active. Preserve sub-action, if exists.
				Agent.Mode = ENavigationMode::Surface;
				if (PrevPoly != Agent.Corridor.getFirstPoly())
					Agent.pNavQuery->getAttachedNavMesh()->getPolyArea(Agent.Corridor.getFirstPoly(), &Agent.CurrAreaType);
				return;
			}
		}
	}
	else
	{
		// Agent is idle or moves along the offmesh connection, check the current poly validity
		if (Agent.pNavQuery->isValidPolyRef(Agent.Corridor.getFirstPoly(), Agent.pNavFilter)) return;

		// If offmesh connection became invalid, cancel its traversal and fail navigation task
		if (Agent.Mode == ENavigationMode::Offmesh)
		{
			// TODO: fail navigation, reset corridor, cancel traversal sub-action, its system will handle cancelling from the middle
			Agent.Mode = ENavigationMode::Recovery;
			return;
		}
	}

	// Our current poly is unknown or invalid, find the nearest valid one
	const float RecoveryRadius = std::min(Agent.Radius * 2.f, 20.f);
	const float RecoveryExtents[3] = { RecoveryRadius, Agent.Height, RecoveryRadius };
	dtPolyRef NearestRef = 0;
	float NearestPos[3];
	Agent.pNavQuery->findNearestPoly(Position.v, RecoveryExtents, Agent.pNavFilter, &NearestRef, NearestPos);

	if (!NearestRef)
	{
		// No poly found in a recovery radius, agent can't recover and needs external position change
		// TODO: fail navigation, reset corridor, cancel traversal sub-action, its system will handle cancelling from the middle
		Agent.Mode = ENavigationMode::Recovery;
		Agent.Corridor.reset(0, Position.v);
		return;
	}

	// Check if our new location is far enough from the navmesh to enable recovery mode
	const float RecoveryThreshold = Agent.Radius * 0.1f;
	if (dtVdist2DSqr(Position.v, NearestPos) > RecoveryThreshold * RecoveryThreshold)
	{
		// TODO: must cancel current traversal sub-action even without replanning, if was not already recovering
		Agent.Mode = ENavigationMode::Recovery;
		Agent.CurrAreaType = 0;
	}
	else
	{
		// TODO: finish recovery, if was active
		Agent.Mode = ENavigationMode::Surface;
		Agent.pNavQuery->getAttachedNavMesh()->getPolyArea(NearestRef, &Agent.CurrAreaType);
	}

	if (Agent.State == ENavigationState::Idle)
	{
		Agent.Corridor.reset(NearestRef, NearestPos);
	}
	else
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
}
//---------------------------------------------------------------------

static void UpdateDestination(Navigate& Action, CNavAgentComponent& Agent)
{
	//!!!FIXME: setting, per Navigate action or per entity!
	constexpr float MaxTargetOffset = 0.5f;
	constexpr float MaxTargetOffsetSq = MaxTargetOffset * MaxTargetOffset;

	if (Agent.State != ENavigationState::Idle)
	{
		if (dtVequal(Agent.TargetPos.v, Action._Destination.v))
		{
			// Target remains static but we must check its poly validity anyway
			if (Agent.pNavQuery->isValidPolyRef(Agent.TargetRef, Agent.pNavFilter)) return;
		}
		else if (Agent.State == ENavigationState::Following)
		{
			// Planning finished, can adjust moving target in the corridor
			if (Agent.Corridor.moveTargetPosition(Action._Destination.v, Agent.pNavQuery, Agent.pNavFilter))
			{
				// Check if target matches the corridor or moved not too far from it into impassable zone.
				// Otherwise target is considered teleported and may require replanning (see below).
				const auto OffsetSq = dtVdist2DSqr(Action._Destination.v, Agent.Corridor.getTarget());
				if (OffsetSq < DEFAULT_EPSILON ||
					(Agent.TargetRef == Agent.Corridor.getLastPoly() && OffsetSq <= MaxTargetOffsetSq))
				{
					Agent.TargetRef = Agent.Corridor.getLastPoly();
					Agent.TargetPos = Agent.Corridor.getTarget();
					return;
				}
			}
		}
	}

	// Target poly is unknown or invalid, find the nearest valid one
	const float TargetExtents[3] = { MaxTargetOffset, Agent.Height, MaxTargetOffset };
	Agent.pNavQuery->findNearestPoly(Action._Destination.v, TargetExtents, Agent.pNavFilter, &Agent.TargetRef, Agent.TargetPos.v);

	if (!Agent.TargetRef || dtVdist2DSqr(Action._Destination.v, Agent.TargetPos.v) > MaxTargetOffsetSq)
	{
		// No poly found in a target radius, navigation task is failed
		// TODO: fail navigation, cancel traversal sub-action, its system will handle cancelling from the middle
		if (Agent.State != ENavigationState::Idle)
		{
			Agent.Corridor.reset(Agent.Corridor.getFirstPoly(), Agent.Corridor.getPos());
			Agent.State = ENavigationState::Idle;
		}
	}
	else
	{
		Agent.State = ENavigationState::Requested;
	}
}
//---------------------------------------------------------------------

static CStrID GetTraversalAction(const CNavAgentComponent& Agent, unsigned char AreaType, dtPolyRef PolyRef)
{
	//???process controller if area is controlled?
	// FIXME: IMPLEMENT!!!
	return CStrID("Steer");
}
//---------------------------------------------------------------------

// TODO: use DT_STRAIGHTPATH_ALL_CROSSINGS for controlled area, where each poly can have personal controller and action
static CStrID GenerateTraversalAction(CNavAgentComponent& Agent, vector3& Dest, vector3& NextDest)
{
	const float* pCurrPos = Agent.Corridor.getPos();
	dtStraightPathContext Ctx;
	if (dtStatusSucceed(Agent.pNavQuery->initStraightPathSearch(
			pCurrPos, Agent.Corridor.getTarget(), Agent.Corridor.getPath(), Agent.Corridor.getPathCount(), Ctx)))
		return CStrID::Empty;

	unsigned char Flags;
	unsigned char AreaType;
	dtPolyRef PolyRef;
	dtStatus Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, NextDest.v, &Flags, &AreaType, &PolyRef, DT_STRAIGHTPATH_AREA_CROSSINGS);
	if (dtStatusFailed(Status)) return CStrID::Empty;

	// If no action available, give an agent a chance to trigger an offmesh connection
	CStrID ActionID = GetTraversalAction(Agent, Agent.CurrAreaType, Agent.Corridor.getPath()[0]);
	if (!ActionID && !(Flags & DT_STRAIGHTPATH_OFFMESH_CONNECTION)) return CStrID::Empty;

	// Advance through path points until the traversal action is generated
	do
	{
		// Elongate path edge
		Dest = NextDest;

		// That was the last edge, finish
		if (!dtStatusInProgress(Status)) break;

		// Check entering an offmesh connection
		if (Flags & DT_STRAIGHTPATH_OFFMESH_CONNECTION)
		{
			const float DistanceSq = dtVdist2DSqr(pCurrPos, Dest.v);
			const float OffmeshTriggerRadius = Agent.Radius * 2.f;
			if (DistanceSq < OffmeshTriggerRadius * OffmeshTriggerRadius)
			{
				// Adjust the path over the off-mesh connection.
				// Path validity check will ensure that bad/blocked connections will be replanned.
				// NB: for offmesh actions Dest is offmesh start and NextDest is offmesh end
				dtPolyRef OffmeshRefs[2];
				if (Agent.Corridor.moveOverOffmeshConnection(PolyRef, OffmeshRefs, Dest.v, NextDest.v, Agent.pNavQuery))
				{
					ActionID = GetTraversalAction(Agent, AreaType, PolyRef);
					Agent.Mode = ENavigationMode::Offmesh;
				}
			}
			break;
		}

		// The next path edge requires different traversal action, finish
		if (ActionID != GetTraversalAction(Agent, AreaType, PolyRef)) break;

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

	// Calculate the remaining distance to the navigation target
	//!!!only if required, NOT every frame!
	/*
	while (dtStatusInProgress(Status))
	{
		//???needed only for arrival braking or for smth else too?
		// Can calculate remaining distance to the target, not every frame, may cache corners and recalculate
		// only when dt is big enough or when the next corner not found in the cache
		Status = Agent.pNavQuery->findNextStraightPathPoint(Ctx, ActionNextDest.v, &Flags, &AreaType, &PolyRef, DT_STRAIGHTPATH_AREA_CROSSINGS);
		if (dtStatusFailed(Status)) break;
	}
	*/

	return ActionID;
}
//---------------------------------------------------------------------

static bool CheckCurrentPath(CNavAgentComponent& Agent)
{
	n_assert_dbg(Agent.State != ENavigationState::Idle);

	// No path - no replanning
	if (Agent.State == ENavigationState::Requested || Agent.State == ENavigationState::Idle) return true;

	// If nearby corridor is not valid, replan
	constexpr int CHECK_LOOKAHEAD = 10;
	if (!Agent.Corridor.isValid(CHECK_LOOKAHEAD, Agent.pNavQuery, Agent.pNavFilter)) return false;

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
	// Quick synchronous search towards the goal, limited iteration count
	Agent.pNavQuery->initSlicedFindPath(Agent.Corridor.getFirstPoly(), Agent.TargetRef, Agent.Corridor.getPos(), Agent.TargetPos.v, Agent.pNavFilter);
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

		Agent.AsyncTaskID = PathQueue.Request(Agent.Corridor.getLastPoly(), Agent.TargetRef, Agent.Corridor.getTarget(), Agent.TargetPos.v, Agent.pNavQuery, Agent.pNavFilter);
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

void ProcessNavigation(DEM::Game::CGameWorld& World, float dt, ::AI::CPathRequestQueue& PathQueue)
{
	World.ForEachEntityWith<CNavAgentComponent, DEM::Game::CActionQueueComponent, const DEM::Game::CSceneComponent>(
		[dt, &PathQueue](auto EntityID, auto& Entity,
			CNavAgentComponent& Agent,
			DEM::Game::CActionQueueComponent* pActions,
			const DEM::Game::CSceneComponent* pSceneComponent)
	{
		if (!pSceneComponent->RootNode || !Agent.pNavQuery || !Agent.pNavFilter) return;

		//!!!Set idle: change state, reset intermediate data, handle breaking in the middle of the offmesh connection
		//???can avoid recalculating straight path edges every frame? do on traversal end or pos changed or corridor invalid?

		//!!!recovery subtask can use speed "as fast as possible" or "recovery" type, then character controller can
		// resolve small length recovery as collision, immediately moving the character to the recovery destination.

		if (auto pNavigateAction = pActions->FindActive<Navigate>())
		{
			// NB: in Detour replan time increases only when on navmesh (not offmesh, not invalid)
			Agent.ReplanTime += dt;

			//???check traversal sub-action result before all other? if failed, fail navigation task or replan.
			// if succeeded and was offmesh, return to navmesh, and only after it update position.

			UpdatePosition(pSceneComponent->RootNode->GetWorldPosition(), Agent);

			//???if position failed Navigate task, early exit?
			//!!!if invalid pos (first corridor poly is zero) exit? can't recover from that pos to the corridor.

			UpdateDestination(*pNavigateAction, Agent);

			//???if destination failed Navigate task (incl. zero target poly ref), early exit?

			//!!!if actor is already at the new destination, finish Navigate action (success), set idle and exit

			// Check current path validity, replan if can't continue using it
			if (!CheckCurrentPath(Agent))
				Agent.State = ENavigationState::Requested;

			if (Agent.State == ENavigationState::Requested)
			{
				//???!!!TODO: cancel traversal sub-action, if any?!

				RequestPath(Agent, PathQueue);
			}
			else if (Agent.State == ENavigationState::Planning)
			{
				if (!CheckAsyncPathResult(Agent, PathQueue))
				{
					// TODO: fail navigation, reset corridor, cancel traversal sub-action, its system will handle cancelling from the middle
					Agent.State = ENavigationState::Idle;
					return;
				}
			}

			if (Agent.Mode == ENavigationMode::Recovery)
			{
				// Try to return to the navmesh by the shortest path

				// add recovery steering action (might be fast-forwarded by the character controller or executed as normal movement)
			}
			else if (Agent.Mode == ENavigationMode::Surface)
			{
				Agent.PathOptimizationTime += dt;

				// FIXME: only if necessary events happened? like offsetting from expected position.
				// The more inaccurate the agent movement, the more beneficial this function becomes. (c) Docs
				// The same for optimizePathVisibility (used below).
				constexpr float OPT_TIME_THR_SEC = 2.5f;
				const bool OptimizePath = (Agent.PathOptimizationTime >= OPT_TIME_THR_SEC);
				if (OptimizePath)
				{
					Agent.Corridor.optimizePathTopology(Agent.pNavQuery, Agent.pNavFilter);
					Agent.PathOptimizationTime = 0.f;
				}

				vector3 ActionDest;
				vector3 ActionNextDest;
				if (auto ActionID = GenerateTraversalAction(Agent, ActionDest, ActionNextDest))
				{
					// get our immediate sub-action from pActions
					// if type != ActionID, reset the stack to Navigate
					// if no sub-action now, create one, else get existing
					// fill dest & next dest in the action, fill other necessary data

					//ActionID;
					//ActionDest;
					//ActionNextDest;
					//!!!for doors etc need poly info to get associated smart object!

					// Use our traversal target to optimize the path, because we know that there is a straight way to it
					if (OptimizePath)
						Agent.Corridor.optimizePathVisibility(ActionDest.v, 30.f * Agent.Radius, Agent.pNavQuery, Agent.pNavFilter);
				}
				else
				{
					// straight path calculation failed
					//???fail navigation? set Idle?
					Agent.Mode = ENavigationMode::Surface; // If the reason is an invalid offmesh connection
				}
			}
		}
		else if (Agent.State != ENavigationState::Idle)
		{
			// set idle
			//???need UpdatePosition once? or need UpdatePosition even when no Navigate action?
		}
	});
}
//---------------------------------------------------------------------

}
