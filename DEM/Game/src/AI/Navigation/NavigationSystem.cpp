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
static inline bool dtVequal2D(const float* p0, const float* p1)
{
	static const float thr = dtSqr(1.0f / 16384.0f);
	const float d = dtVdist2DSqr(p0, p1);
	return d < thr;
}
//---------------------------------------------------------------------

static void UpdatePosition(const vector3& Position, CNavigationComponent& Navigation, bool& Replan)
{
	const bool PositionChanged = !dtVequal2D(Navigation.Corridor.getPos(), Position.v);

	//???!!!movePosition etc only if not idle?

	if (PositionChanged)
	{
		if (Navigation.Mode == ENavigationMode::Offmesh)
		{
			// Not moving along the navmesh surface
			//!!!if traversing offmesh, must check its poly validity! see (!PositionChanged) below, the same is done!
		}
		else
		{
			// Try to move along the navmesh surface to the new position
			if (Navigation.Corridor.movePosition(Position.v, Navigation.pNavQuery, Navigation.pNavFilter))
			{
				if (dtVequal2D(Position.v, Navigation.Corridor.getPos()))
				{
					Navigation.Mode = ENavigationMode::Surface;
				}
				else
				{
					// Moved too far or off the navmesh surface
					const float Extents[3] = { 0.f, pActor->Height, 0.f };
					dtPolyRef Ref = 0;
					float Nearest[3];
					Navigation.pNavQuery->findNearestPoly(Position.v, Extents, Navigation.pNavFilter, &Ref, Nearest);

					if (Ref && dtVequal2D(Position.v, Nearest))
					{
						// We are on the navmesh, but our corridor needs rebuilding
						Navigation.Mode = ENavigationMode::Surface;
						Navigation.Corridor.reset(Ref, Position.v);
						Replan = true;
					}
					else
					{
						Navigation.Mode = ENavigationMode::Recovery;
					}
				}
			}
			else
			{
				// Invalid starting poly or args
				Navigation.Mode = ENavigationMode::Recovery;
			}

			//!!!and/or trigger an offmesh connection here!
			//if triggered offmesh, finish current edge traversal!

			//!!!if corridor pos != agent pos, need to set invalid state and recovery!
		}
	}
	else
	{
		// Position is the same, but poly might become invalid
		if (!Navigation.pNavQuery->isValidPolyRef(Navigation.Corridor.getFirstPoly(), Navigation.pNavFilter))
		{
			// find another poly under the feet, it may be valid
			//???use fixPathStart or do as above?

			if (Navigation.State == ENavigationState::Idle)
			{
				//???end edge traversal? must ensure that offmesh connection will be processed correctly!
				Navigation.Corridor.reset(0, Position.v);
			}
			else
			{
				Navigation.Mode = ENavigationMode::Recovery;
			}
		}
	}

	// if valid and not idle (and position changed?) (and not replanning?), check current edge traversal end


	///////////////////////////////

	if (Navigation.State == ENavigationState::Idle)
	{
		if (PositionChanged)
		{
			// reset poly
			// single-poly corridor: Corridor.reset(Corridor.getFirstPoly(), Corridor.getPos());
		}
	}
	else
	{
		//!!!instead of this condition detect entering offmesh connection!
		if (Navigation.Mode == ENavigationMode::Offmesh)
		{
			//???!!!always call moveOverOffmeshConnectionat start? then wait reaching offmesh connection end
			// if started or ended offmesh traversal and this side is "done", moveOverOffmeshConnection
		}
		else
		{
			// FIXME: if moved too far AND starting poly changed, better to replan (or check return false?)
			Navigation.Corridor.movePosition(Position.v, Navigation.pNavQuery, Navigation.pNavFilter);
			// else movePosition, if failed update position poly
		}

		// check if edge traversal sub-action is done, finalize it
		// detect both normal path edge and offmesh connection arrivals

		// DT:
		// if not on offmesh
		//   if curr poly invalid, find nearest poly and navmesh point in a target recovery radius
		//     if not found, reset corridor and drop navigation task
		//     else remember new destination point (in corridor is enough?) and replan
	}

	// set position validity flag (real == corridor, in other words real is on navmesh)
}
//---------------------------------------------------------------------

static void UpdateDestination(const vector3& Destination, CNavigationComponent& Navigation, bool& Replan)
{
	const bool TargetChanged = !dtVequal(Navigation.Destination.v, Destination.v);

	// FIXME: poly validity check still must happen!
	//if (Navigation.State != ENavigationState::Idle && !TargetChanged) return;

	Navigation.Destination = Destination;

	// FIXME: if moved too far AND target poly changed, better to replan (or check return false?)
	// if not idle, moveTargetPosition
	// else set requested, no replan

	// update dest poly if needed
	// if dest is invalid, fix to navmesh and chande desired destination, may need replan

	// DT:
	// if not idle and not on offmesh
	//   if target poly invalid, find nearest poly in a recovery radius
	//     if on found poly, corridor.fixPathStart and replan
	//     else reset corridor to zero poly and go into invalid state
}
//---------------------------------------------------------------------

void ProcessNavigation(DEM::Game::CGameWorld& World)
{
	World.ForEachEntityWith<CNavigationComponent, DEM::Game::CActionQueueComponent, const DEM::Game::CSceneComponent>(
		[](auto EntityID, auto& Entity,
			CNavigationComponent& Navigation,
			DEM::Game::CActionQueueComponent* pActions,
			const DEM::Game::CSceneComponent* pSceneComponent)
	{
		if (!pSceneComponent->RootNode) return;

		//!!!Set idle: change state, reset intermediate data, handle breaking in the middle of the offmesh connection
		//???can avoid recalculating straight path edges every frame? do on traversal end or pos changed or corridor invalid?

		if (auto pNavigateAction = pActions->FindActive<Navigate>())
		{
			//!!!TODO:
			// update time since last replan and other timers, if based on elapsed time
			// NB: in Detour replan time increases only when on navmesh (not offmesh, not invalid)

			bool Replan = false;
			const auto& Position = pSceneComponent->RootNode->GetWorldPosition();
			UpdatePosition(Position, Navigation, Replan);
			UpdateDestination(pNavigateAction->_Destination, Navigation, Replan);

			// if actor is already at the new destination, finish Navigate action (success), set idle and exit
			// if navigation failed, finish Navigate action (failure), set idle and exit
			//???if position is invalid and recovery point not found, finish Navigate action (failure), set idle and exit?

			// validate corridor CHECK_LOOKAHEAD polys ahead, if failed replan
			// if following path not waiting anything, and end is near and it's not target, replan
			if (Replan)
			{
				//request replanning
			}

			// if state is REQUEST
			//  initSlicedFindPath + updateSlicedFindPath(few iterations) for quickpath
			//  finalizeSlicedFindPath[Partial if replan]
			//  if finalized and not empty, set corridor to partial or full path, partial requires target adjusting
			//  else reset corridor to the current position (will wait for full path)
			//  if full path, set state VALID
			//  else run async request and set state WAIT_ASYNC_PATH

			//!!!async queue runs outside here! or could manage async request inside the agent itself, but multithreading
			// then will be enabled only if the current function will be dispatched across different threads. Also total
			// balancing prevents async request to be managed here!!! Need external!

			// if WAIT_ASYNC_PATH (may check only if was not just set!)
			//   get request state
			//   if failed, retry REQUEST if target is valid or set FAILED if not
			//   else if done
			//     get result, fail if empty or not retrieved or curr end != new start
			//     if has curr path, merge with new and remove trackbacks
			//     set corridor to partial or full path, partial requires target adjusting (FAILED if fail)
			//     set state VALID

			// if on navmesh and has target
			//   optimizePathTopology if it is the time and/or necessary events happened
			//   findCorners / findStraightPath
			//   optimizePathVisibility if it is the time and/or necessary events happened
			//   if in trigger range of an offmesh connection, moveOverOffmeshConnection and set OFFMESH state
			//   create sub-action for current edge traversal

			//==============

			// if corridor is invalid, trim and fix or even do full replanning
			// if we are on the navmesh and are nearing corridor end poly which is not target poly and replan time has come, replan
			// if replan flag is set after all above, we will do replanning this frame
			// replanning:
			//   try calc quick path
			//   if found, use quick path and build remaining part in parallel
			//   else find from scratch
			// if quick path was not full:
			//   if no async request, run one
			//   else check its status
			//     if failed, all navigation is failed
			//     else if done, get result and merge into a quick path and fix curr dest to the final one, setCorridor
			//     clear request info
			// if following full path (no planning awaited):
			//   optimizePathTopology periodicaly (???better on some changes?)

			// <process offmesh traversal - where and how?>
		}
		else if (Navigation.State != ENavigationState::Idle)
		{
			// set idle
		}
	});
}
//---------------------------------------------------------------------

}
