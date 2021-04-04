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

bool CLockpickInteraction::IsAvailable(const Game::CInteractionContext& Context) const
{
	// Need at least one capable actor. No mechanics skill is required because there are locks
	// that are simple enough to be lockpicked by anyone. Hard locks must be filtered in targeting code.
	return !Context.Actors.empty(); //!!! && AnyHasCapability(Context.Actors, ECapability::Interact)
}
//---------------------------------------------------------------------

bool CLockpickInteraction::Execute(Game::CInteractionContext& Context, bool Enqueue) const
{
	if (Context.Targets.empty() || !Context.Targets[0].Entity || Context.Actors.empty()) return false;

	auto pWorld = Context.Session->FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;

	//!!!DBG TMP!
	pWorld->RemoveComponent<CLockComponent>(Context.Targets[0].Entity);

	return true;
}
//---------------------------------------------------------------------

}
