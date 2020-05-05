#include "NavSystem.h"
#include <Game/GameLevel.h>
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <AI/Navigation/NavigationComponent.h>

namespace DEM::AI
{

//!!!Set idle: change state, reset intermediate data, handle breaking in the middle of the offmesh connection
//???can avoid recalculating straight path edges every frame? do on traversal end or pos changed or corridor invalid?

void ProcessNavigation(DEM::Game::CGameWorld& World)
{
	World.ForEachEntityWith<CNavigationComponent, DEM::Game::CActionQueueComponent>(
		[](auto EntityID, auto& Entity, CNavigationComponent& Navigation, DEM::Game::CActionQueueComponent* pActions)
	{
		if (auto pNavigateAction = pActions->FindActive<Navigate>())
		{
			// update time since last replan and other timers, if based on elapsed time

			// update navigation from current actor position
			//   if not idle
			//     if started or ended offmesh traversal and this side is "done", moveOverOffmeshConnection
			//     else movePosition, if failed update position poly
			//     check if edge traversal sub-action is done, finalize it
			//   else
			//     update position poly
			//   set position validity flag (real == corridor, in other words real is on navmesh)

			// if is idle or destination changed
			//   update destination
			//   if not idle, moveTargetPosition
			//   update dest poly if needed
			//   if dest is invalid, fix to navmesh and chande desired destination, may need replan

			// if actor is already at the new destination, finish Navigate action, set idle and exit

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
		else
		{
			// if navigation state is not idle, set idle
		}
	});
}
//---------------------------------------------------------------------

}
