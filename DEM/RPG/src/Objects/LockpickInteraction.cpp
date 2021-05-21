#include "LockpickInteraction.h"
#include <Objects/LockComponent.h>
#include <Game/Interaction/InteractionContext.h>
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>

namespace DEM::RPG
{

CLockpickInteraction::CLockpickInteraction(std::string_view CursorImage)
{
	_Name = "Lockpick";
	_CursorImage = CursorImage;
}
//---------------------------------------------------------------------

bool CLockpickInteraction::IsAvailable(const Game::CInteractionContext& Context) const
{
	// Need at least one capable actor. No mechanics skill is required because there are locks
	// that are simple enough to be lockpicked by anyone. Hard locks must be filtered in targeting code.
	return !Context.Actors.empty(); //!!! && AnyHasCapability(Context.Actors, ECapability::Interact)
}
//---------------------------------------------------------------------

bool IsTargetLocked(const Game::CGameSession& Session, const Game::CInteractionContext& Context, U32 Index)
{
	// Check for the lock component
	const auto& Target = (Index == Context.Targets.size()) ? Context.CandidateTarget : Context.Targets[Index];
	if (!Target.Valid) return false;
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	return pWorld && pWorld->FindComponent<CLockComponent>(Target.Entity);
}
//---------------------------------------------------------------------

bool CLockpickInteraction::IsTargetValid(const Game::CGameSession& Session, U32 Index, const Game::CInteractionContext& Context) const
{
	return Index == 0 && IsTargetLocked(Session, Context, Index);
}
//---------------------------------------------------------------------

ESoftBool CLockpickInteraction::NeedMoreTargets(const Game::CInteractionContext& Context) const
{
	return (Context.Targets.size() < 1) ? ESoftBool::True : ESoftBool::False;
}
//---------------------------------------------------------------------

bool CLockpickInteraction::Execute(Game::CGameSession& Session, Game::CInteractionContext& Context, bool Enqueue, bool PushChild) const
{
	if (Context.Targets.empty() || !Context.Targets[0].Entity || Context.Actors.empty()) return false;

	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;

	//!!!DBG TMP!
	pWorld->RemoveComponent<CLockComponent>(Context.Targets[0].Entity);

	return true;
}
//---------------------------------------------------------------------

}
