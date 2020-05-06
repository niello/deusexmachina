#include "NavSystem.h"
#include <Game/GameLevel.h>
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <Game/ECS/Components/SceneComponent.h>
#include <AI/Navigation/NavigationComponent.h>
#include <DetourCommon.h>

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

static void UpdatePosition(const vector3& Position, CNavigationComponent& Navigation)
{
	const bool PositionChanged = !dtVequal2D(Navigation.Corridor.getPos(), Position.v);

	if (PositionChanged && Navigation.Mode != ENavigationMode::Offmesh && Navigation.State != ENavigationState::Idle)
	{
		// Agent moves along the navmesh surface or recovers to it, adjust the corridor
		if (Navigation.Corridor.movePosition(Position.v, Navigation.pNavQuery, Navigation.pNavFilter))
		{
			if (dtVequal2D(Position.v, Navigation.Corridor.getPos()))
			{
				// TODO: finish recovery, if was active. Preserve sub-action, if exists.
				Navigation.Mode = ENavigationMode::Surface;
				return;
			}
		}
	}
	else
	{
		// Agent is idle or moves along the offmesh connection, check the current poly validity
		if (Navigation.pNavQuery->isValidPolyRef(Navigation.Corridor.getFirstPoly(), Navigation.pNavFilter)) return;

		// If offmesh connection became invalid, cancel its traversal and fail navigation task
		if (Navigation.Mode == ENavigationMode::Offmesh)
		{
			// TODO: fail navigation, reset corridor, cancel traversal sub-action, its system will handle cancelling from the middle
			Navigation.Mode = ENavigationMode::Recovery;
			return;
		}
	}

	// Our current poly is unknown or invalid, find the nearest valid one
	const float RecoveryRadius = std::min(pActor->Radius * 2.f, 20.f);
	const float RecoveryExtents[3] = { RecoveryRadius, pActor->Height, RecoveryRadius };
	dtPolyRef NearestRef = 0;
	float NearestPos[3];
	Navigation.pNavQuery->findNearestPoly(Position.v, RecoveryExtents, Navigation.pNavFilter, &NearestRef, NearestPos);

	if (!NearestRef)
	{
		// No poly found in a recovery radius, agent can't recover and needs external position change
		// TODO: fail navigation, reset corridor, cancel traversal sub-action, its system will handle cancelling from the middle
		Navigation.Mode = ENavigationMode::Recovery;
		Navigation.Corridor.reset(0, Position.v);
		return;
	}

	// Check if our new location is far enough from the navmesh to enable recovery mode
	const float RecoveryThreshold = pActor->Radius * 0.1f;
	if (dtVdist2DSqr(Position.v, NearestPos) > RecoveryThreshold * RecoveryThreshold)
	{
		// TODO: must cancel current traversal sub-action even without replanning, if was not already recovering
		Navigation.Mode = ENavigationMode::Recovery;
	}
	else
	{
		// TODO: finish recovery, if was active
		Navigation.Mode = ENavigationMode::Surface;
	}

	if (Navigation.State == ENavigationState::Idle)
	{
		Navigation.Corridor.reset(NearestRef, NearestPos);
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
		Navigation.Corridor.fixPathStart(NearestRef, NearestPos);
		Navigation.State = ENavigationState::Requested;
	}
}
//---------------------------------------------------------------------

static void UpdateDestination(Navigate& Action, CNavigationComponent& Navigation)
{
	//!!!FIXME: setting, per Navigate action or per entity!
	constexpr float MaxTargetOffset = 0.5f;
	constexpr float MaxTargetOffsetSq = MaxTargetOffset * MaxTargetOffset;

	if (Navigation.State != ENavigationState::Idle)
	{
		if (dtVequal(Navigation.TargetPos.v, Action._Destination.v))
		{
			// Target remains static but we must check its poly validity anyway
			if (Navigation.pNavQuery->isValidPolyRef(Navigation.TargetRef, Navigation.pNavFilter)) return;
		}
		else if (Navigation.State == ENavigationState::Following)
		{
			// Planning finished, can adjust moving target in the corridor
			if (Navigation.Corridor.moveTargetPosition(Action._Destination.v, Navigation.pNavQuery, Navigation.pNavFilter))
			{
				// Check if target matches the corridor or moved not too far from it into impassable zone.
				// Otherwise target is considered teleported and may require replanning (see below).
				const auto OffsetSq = dtVdist2DSqr(Action._Destination.v, Navigation.Corridor.getTarget());
				if (OffsetSq < DEFAULT_EPSILON ||
					(Navigation.TargetRef == Navigation.Corridor.getLastPoly() && OffsetSq <= MaxTargetOffsetSq))
				{
					Navigation.TargetRef = Navigation.Corridor.getLastPoly();
					Navigation.TargetPos = Navigation.Corridor.getTarget();
					return;
				}
			}
		}
	}

	// Target poly is unknown or invalid, find the nearest valid one
	const float TargetExtents[3] = { MaxTargetOffset, pActor->Height, MaxTargetOffset };
	Navigation.pNavQuery->findNearestPoly(Action._Destination.v, TargetExtents, Navigation.pNavFilter, &Navigation.TargetRef, Navigation.TargetPos.v);

	if (!Navigation.TargetRef || dtVdist2DSqr(Action._Destination.v, Navigation.TargetPos.v) > MaxTargetOffsetSq)
	{
		// No poly found in a target radius, navigation task is failed
		// TODO: fail navigation, cancel traversal sub-action, its system will handle cancelling from the middle
		if (Navigation.State != ENavigationState::Idle)
		{
			Navigation.Corridor.reset(Navigation.Corridor.getFirstPoly(), Navigation.Corridor.getPos());
			Navigation.State = ENavigationState::Idle;
		}
	}
	else
	{
		Navigation.State = ENavigationState::Requested;
	}
}
//---------------------------------------------------------------------

static bool CheckCurrentPath(CNavigationComponent& Navigation)
{
	n_assert_dbg(Navigation.State != ENavigationState::Idle);

	// No path - no replanning
	if (Navigation.State == ENavigationState::Requested || Navigation.State == ENavigationState::Idle) return true;

	// If nearby corridor is not valid, replan
	constexpr int CHECK_LOOKAHEAD = 10;
	if (!Navigation.Corridor.isValid(CHECK_LOOKAHEAD, Navigation.pNavQuery, Navigation.pNavFilter)) return false;

	// If the end of the path is near and it is not the requested location, replan
	constexpr float TARGET_REPLAN_DELAY = 1.0f; // seconds
	if (Navigation.State == ENavigationState::Following &&
		Navigation.ReplanTime > TARGET_REPLAN_DELAY &&
		Navigation.Corridor.getPathCount() < CHECK_LOOKAHEAD &&
		Navigation.Corridor.getLastPoly() != Navigation.TargetRef)
	{
		return false;
	}

	return true;
}
//---------------------------------------------------------------------

void ProcessNavigation(DEM::Game::CGameWorld& World, float dt)
{
	World.ForEachEntityWith<CNavigationComponent, DEM::Game::CActionQueueComponent, const DEM::Game::CSceneComponent>(
		[dt](auto EntityID, auto& Entity,
			CNavigationComponent& Navigation,
			DEM::Game::CActionQueueComponent* pActions,
			const DEM::Game::CSceneComponent* pSceneComponent)
	{
		if (!pSceneComponent->RootNode) return;

		//!!!Set idle: change state, reset intermediate data, handle breaking in the middle of the offmesh connection
		//???can avoid recalculating straight path edges every frame? do on traversal end or pos changed or corridor invalid?

		//!!!recovery subtask can use speed "as fast as possible" or "recovery" type, then character controller can
		// resolve small length recovery as collision, immediately moving the character to the recovery destination.

		if (auto pNavigateAction = pActions->FindActive<Navigate>())
		{
			// NB: in Detour replan time increases only when on navmesh (not offmesh, not invalid)
			Navigation.ReplanTime += dt;

			//???check traversal sub-action result before all other? if failed, fail navigation task or replan.
			// if succeeded and was offmesh, return to navmesh.

			UpdatePosition(pSceneComponent->RootNode->GetWorldPosition(), Navigation);

			//???if position failed Navigate task, early exit?
			//!!!if invalid pos (first corridor poly is zero) exit? can't recover from that pos to the corridor.

			UpdateDestination(*pNavigateAction, Navigation);

			//???if destination failed Navigate task (incl. zero target poly ref), early exit?

			//!!!if actor is already at the new destination, finish Navigate action (success), set idle and exit

			// Check current path validity, replan if can't continue using it
			if (!CheckCurrentPath(Navigation))
				Navigation.State = ENavigationState::Requested;

			if (Navigation.State = ENavigationState::Requested)
			{
				//???!!!TODO: cancel traversal sub-action, if any?!

				// Quick synchronous search towards the goal, limited iteration count
				Navigation.pNavQuery->initSlicedFindPath(Navigation.Corridor.getFirstPoly(), Navigation.TargetRef, Navigation.Corridor.getPos(), Navigation.TargetPos.v, Navigation.pNavFilter);
				Navigation.pNavQuery->updateSlicedFindPath(20, 0);

				// Try to reuse existing steady path if possible and worthwile
				std::array<dtPolyRef, 32> Path;
				int PathSize = 0;
				dtStatus Status = (Navigation.Corridor.getPathCount() > 10) ?
					Navigation.pNavQuery->finalizeSlicedFindPathPartial(Navigation.Corridor.getPath(), Navigation.Corridor.getPathCount(), Path.data(), &PathSize, Path.size()) :
					Navigation.pNavQuery->finalizeSlicedFindPath(Path.data(), &PathSize, Path.size());

				// Setup path corridor
				const dtPolyRef LastPoly = (dtStatusFailed(Status) || !PathSize) ? 0 : Path[PathSize - 1];
				if (LastPoly == Navigation.TargetRef)
				{
					// Full path
					Navigation.Corridor.setCorridor(Navigation.TargetPos, Path.data(), PathSize);
					Navigation.State = ENavigationState::Following;
					Navigation.ReplanTime = 0.f;
				}
				else
				{
					// Partial path, constrain target position inside the last polygon or reset to agent position
					float PartialTargetPos[3];
					if (LastPoly && dtStatusSucceed(Navigation.pNavQuery->closestPointOnPoly(LastPoly, Navigation.TargetPos, PartialTargetPos, nullptr)))
						Navigation.Corridor.setCorridor(PartialTargetPos, Path.data(), PathSize);
					else
						Navigation.Corridor.setCorridor(Navigation.Corridor.getPos(), Navigation.Corridor.getFirstPoly(), 1);

					//!!!TODO: use CPathRequestQueue!?
					Navigation.AsyncTaskID = m_pathq.request(Navigation.Corridor.getLastPoly(), Navigation.TargetRef, Navigation.Corridor.getTarget(), Navigation.TargetPos, Navigation.pNavFilter);
					if (Navigation.AsyncTaskID)
						Navigation.State = ENavigationState::Planning;
				}
			}
			else if (Navigation.State == ENavigationState::Planning)
			{
				//   get request state
				//   if failed, retry REQUEST if target is valid or set FAILED if not
				//   else if done
				//     get result, fail if empty or not retrieved or curr end != new start
				//     if has curr path, merge with new and remove trackbacks
				//     set corridor to partial or full path, partial requires target adjusting (FAILED if fail)
				//     set state VALID
			}

			//!!!async queue runs outside here! or could manage async request inside the agent itself, but multithreading
			// then will be enabled only if the current function will be dispatched across different threads. Also total
			// balancing prevents async request to be managed here!!! Need external!

			if (Navigation.Mode != ENavigationMode::Offmesh)
			{
				//   optimizePathTopology if it is the time and/or necessary events happened
				//   findCorners / findStraightPath
				//   optimizePathVisibility if it is the time and/or necessary events happened
				//   if in trigger range of an offmesh connection, moveOverOffmeshConnection and set OFFMESH state
				//   create/update/check sub-action for current edge traversal
			}
		}
		else if (Navigation.State != ENavigationState::Idle)
		{
			// set idle
			//???need UpdatePosition?
		}
	});
}
//---------------------------------------------------------------------

}
