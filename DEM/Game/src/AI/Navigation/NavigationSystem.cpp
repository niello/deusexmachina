#include "NavSystem.h"
#include <Game/GameLevel.h>
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <AI/Navigation/NavigationComponent.h>

namespace DEM::AI
{

void PlanPath(DEM::Game::CGameWorld& World)
{
	World.ForEachEntityWith<CNavigationComponent, DEM::Game::CActionQueueComponent>(
		[](auto EntityID, auto& Entity, CNavigationComponent& Navigation, DEM::Game::CActionQueueComponent* pActions)
	{
		auto pNavigateAction = pActions->FindActive<Navigate>();

		//!!!if not found, must turn navigation system idle and reset component state!
		// Check if an actor must navigate now
		//if (!pNavigateAction) return;

		//???can understand that request is the same? or always compare destination? per-frame but still not awfully slow

		// Logic from CNavSystem::SetDestPoint():
		// if new dest is the same as current and state is not Idle, return
		// set new dest
		// if already there, finish Navigate
		// if has prev target and poly not changed, update target in corridor
		// else reset dest poly, and if dest OK but pos invalid, reset pos poly

		// Logic from CNavSystem::UpdatePosition():
		// if reached offmesh connection enter circle, moveOverOffmeshConnection (or call it when end traversal action???)
		// if not offmesh, and idle, reset pos. poly
		// if not offmesh and not idle, movePosition, reset poly if failed, check arrival to the final dest

		// Logic from CNavSystem::Update():
		// count replan time
		// check and remember pos validity - actual == corridor
		// if nav system idle, return
		// process offmesh connection traversal, breaking in the middle etc
		// if dest invalid, reset target poly, exit if became idle, mark for replanning
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

		// EndEdgeTraversal - check if sub-action is finished

		//???can avoid recalculating straight path edges every frame? do on traversal end or pos changed or corridor invalid?

		// using context from pNavigation, update path planning and finish with setting up a nested action or with removing Navigate
	});
}
//---------------------------------------------------------------------

}
