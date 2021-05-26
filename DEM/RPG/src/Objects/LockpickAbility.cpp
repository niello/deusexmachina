#include "LockpickAbility.h"
#include <Objects/LockComponent.h>
#include <Animation/AnimationComponent.h>
#include <Animation/AnimationController.h>
#include <Game/Interaction/AbilityInstance.h>
#include <Game/ECS/GameWorld.h>

namespace DEM::RPG
{

CLockpickAbility::CLockpickAbility(std::string_view CursorImage)
{
	_Name = "Lockpick";
	_CursorImage = CursorImage;
}
//---------------------------------------------------------------------

// TODO:
// check a capability to interact
bool CLockpickAbility::IsAvailable(const Game::CInteractionContext& Context) const
{
	// Need at least one capable actor. No mechanics skill is required because there are locks
	// that are simple enough to be lockpicked by anyone. Hard locks must be filtered in targeting code.
	return !Context.Actors.empty(); //!!! && AnyHasCapability(Context.Actors, ECapability::Interact)
}
//---------------------------------------------------------------------

static bool IsTargetLocked(const Game::CGameSession& Session, const Game::CInteractionContext& Context, U32 Index)
{
	// Check for the lock component
	const auto& Target = (Index == Context.Targets.size()) ? Context.CandidateTarget : Context.Targets[Index];
	if (!Target.Valid) return false;
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;
	auto pLock = pWorld->FindComponent<CLockComponent>(Target.Entity);
	return pLock && !pLock->Jamming;
}
//---------------------------------------------------------------------

// TODO:
// if no lock or lock is jammed, fail
// if lock is not trivial, require skill > 0 and lockpicking tools item as a source
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

// TODO:
// choose the actor able to lockpick with the best chance (max bonus)
// (optional) choose an actor able to lockpick with second max bonus, not less than some X, make him move and perform assistance
bool CLockpickAbility::Execute(Game::CGameSession& Session, Game::CInteractionContext& Context, bool Enqueue, bool PushChild) const
{
	if (Context.Targets.empty() || !Context.Targets[0].Entity || Context.Actors.empty()) return false;

	// Push standard action for the first actor only
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

// TODO:
// roll dice, add stat, skill, item and assistant modifiers, remember result
// set animation based on that result - failed, succeeded, jammed
// if target belongs to another faction, create crime stimulus based on character's stealth and lockpicking difficulty
void CLockpickAbility::OnStart(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const
{
	if (auto pWorld = Session.FindFeature<Game::CGameWorld>())
		if (auto pAnimComponent = pWorld->FindComponent<Game::CAnimationComponent>(Instance.Actor))
			pAnimComponent->Controller.SetString(CStrID("Action"), CStrID("TryOpenDoor")); // FIXME: need correct ID!
}
//---------------------------------------------------------------------

// TODO:
// play some sounds
// wait with playing anim, duration is based on result: failed = 1.5-2 sec, succeeded = 3-5 sec, jammed = 3-7 sec
// (optional) wait for assistant before start, then perform what's now in OnStart
// (optional) check if assistant cancelled its action, and if so, remove its bonus
Game::EActionStatus CLockpickAbility::OnUpdate(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const
{
	return (Instance.ElapsedTime >= 2.f) ? Game::EActionStatus::Succeeded : Game::EActionStatus::Active;
}
//---------------------------------------------------------------------

// TODO:
// change lock status - leave as is, destroy or set Jamming value based on the dice roll result
// play result sound from the lock
// if character is not in stealth, play result phrase
// remove crime stimulus
void CLockpickAbility::OnEnd(Game::CGameSession& Session, Game::CAbilityInstance& Instance, Game::EActionStatus Status) const
{
	if (auto pWorld = Session.FindFeature<Game::CGameWorld>())
	{
		if (auto pAnimComponent = pWorld->FindComponent<Game::CAnimationComponent>(Instance.Actor))
			pAnimComponent->Controller.SetString(CStrID("Action"), CStrID::Empty);

		//!!!DBG TMP!
		if (Status == Game::EActionStatus::Succeeded)
			pWorld->RemoveComponent<CLockComponent>(Instance.Targets[0].Entity);
	}
}
//---------------------------------------------------------------------

}
