#include "LockpickAbility.h"
#include <Objects/LockComponent.h>
#include <Game/Interaction/InteractionContext.h>
#include <Game/Interaction/AbilityInstance.h>
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>

namespace DEM::RPG
{

CLockpickAbility::CLockpickAbility(std::string_view CursorImage)
{
	_Name = "Lockpick";
	_CursorImage = CursorImage;
}
//---------------------------------------------------------------------

bool CLockpickAbility::IsAvailable(const Game::CInteractionContext& Context) const
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

bool CLockpickAbility::IsTargetValid(const Game::CGameSession& Session, U32 Index, const Game::CInteractionContext& Context) const
{
	return Index == 0 && IsTargetLocked(Session, Context, Index);
}
//---------------------------------------------------------------------

ESoftBool CLockpickAbility::NeedMoreTargets(const Game::CInteractionContext& Context) const
{
	return (Context.Targets.size() < 1) ? ESoftBool::True : ESoftBool::False;
}
//---------------------------------------------------------------------

bool CLockpickAbility::Execute(Game::CGameSession& Session, Game::CInteractionContext& Context, bool Enqueue, bool PushChild) const
{
	if (Context.Targets.empty() || !Context.Targets[0].Entity || Context.Actors.empty()) return false;

	// Push standard action for the first actor only
	// TODO: choose the first actor able to lockpick, or even the actor with the best chance!
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	return pWorld && PushStandardExecuteAction(*pWorld, Context.Actors[0], Context, Enqueue, PushChild);
}
//---------------------------------------------------------------------

bool CLockpickAbility::GetFacingParams(const Game::CGameSession& Session, const Game::CAbilityInstance& Instance, Game::CFacingParams& Out) const
{
	// For now simply face an object origin
	Out.Mode = Game::EFacingMode::Point;
	Out.Dir = vector3::Zero;
	Out.Tolerance = 1.5f;
	return true;
}
//---------------------------------------------------------------------

void CLockpickAbility::OnStart(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const
{
}
//---------------------------------------------------------------------

Game::EActionStatus CLockpickAbility::OnUpdate(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const
{
	return (Instance.ElapsedTime >= 2.f) ? Game::EActionStatus::Succeeded : Game::EActionStatus::Active;
}
//---------------------------------------------------------------------

void CLockpickAbility::OnEnd(Game::CGameSession& Session, Game::CAbilityInstance& Instance, Game::EActionStatus Status) const
{
	//!!!DBG TMP!
	if (Status == Game::EActionStatus::Succeeded)
		if (auto pWorld = Session.FindFeature<Game::CGameWorld>())
			pWorld->RemoveComponent<CLockComponent>(Instance.Targets[0].Entity);
}
//---------------------------------------------------------------------

}
