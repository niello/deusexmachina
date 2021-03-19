#include "LockpickInteraction.h"
#include <Objects/LockedTargetFilter.h>
#include <Game/ECS/Components/ActionQueueComponent.h>

namespace DEM::RPG
{

CLockpickInteraction::CLockpickInteraction(std::string_view CursorImage)
{
	_Name = "Lockpick";
	_CursorImage = CursorImage;

	AddTarget(std::make_unique<CLockedTargetFilter>());
}
//---------------------------------------------------------------------

bool CLockpickInteraction::Execute(Game::CInteractionContext& Context, bool Enqueue) const
{
	if (Context.SelectedTargets.empty() || !Context.SelectedTargets[0].Entity || Context.SelectedActors.empty()) return false;

	auto pWorld = Context.Session->FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;

	//!!!DBG TMP!
	pWorld->RemoveComponent<CLockComponent>(Context.SelectedTargets[0].Entity);

	return true;
}
//---------------------------------------------------------------------

}
