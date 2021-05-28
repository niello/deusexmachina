#include "LockpickAbility.h"
#include <Objects/LockComponent.h>
#include <Animation/AnimationComponent.h>
#include <Animation/AnimationController.h>
#include <Game/Interaction/AbilityInstance.h>
#include <Game/ECS/GameWorld.h>
#include <Character/StatsComponent.h>
#include <Character/SkillsComponent.h>
#include <Items/LockpickComponent.h>

namespace DEM::RPG
{

CLockpickAbility::CLockpickAbility(std::string_view CursorImage)
{
	_Name = "Lockpick";
	_CursorImage = CursorImage;
}
//---------------------------------------------------------------------

bool CLockpickAbility::IsAvailable(const Game::CGameSession& Session, const Game::CInteractionContext& Context) const
{
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;

	// Need at least one capable actor. No mechanics skill is required because there are locks that are
	// simple enough to be lockpicked by anyone. Hard locks must be filtered in target validation code.
	// TODO: check character caps - to utility function?
	for (auto ActorID : Context.Actors)
		if (auto pStats = pWorld->FindComponent<Sh2::CStatsComponent>(ActorID))
			if (pStats->Capabilities & Sh2::ECapability::Interact) return true;

	return false;
}
//---------------------------------------------------------------------

bool CLockpickAbility::IsTargetValid(const Game::CGameSession& Session, U32 Index, const Game::CInteractionContext& Context) const
{
	// Ability accepts only one target
	if (Index != 0) return false;

	// Check for the lock component
	const auto& Target = (Index == Context.Targets.size()) ? Context.CandidateTarget : Context.Targets[Index];
	if (!Target.Valid) return false;
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;
	auto pLock = pWorld->FindComponent<CLockComponent>(Target.Entity);

	// TODO: if jammed, must show an action as disabled with a reason, not skip it
	if (!pLock || pLock->Jamming) return false;

	// Trivial locks can be picked without tools and skill
	// TODO: move constant somewhere (Sh2::Balance::GetValue(ID)?)
	if (pLock->Difficulty < 5) return true;

	// Must use tool for non-trivial locks
	//!!!FIXME: tool may be not held by an actor who will pick the lock!!!
	//???for multiple actors find a lockpick in all equipment? isn't this all an overcomplication?
	if (!pWorld->FindComponent<Sh2::CLockpickComponent>(Context.Source)) return false;

	// An actor must be able to interact and must have a lockpicking skill opened
	for (auto ActorID : Context.Actors)
		if (auto pStats = pWorld->FindComponent<Sh2::CStatsComponent>(ActorID))
			if (pStats->Capabilities & Sh2::ECapability::Interact)
				if (auto pSkills = pWorld->FindComponent<Sh2::CSkillsComponent>(ActorID))
					if (pSkills->Lockpicking > 0) return true;

	// TODO: must show an action as disabled with a reason, not skip it
	return false;
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

	// TODO:
	// choose the actor able to lockpick with the best chance (max bonus). DEX modifier, skill, tools.
	// (optional) choose an actor able to lockpick with second max bonus, not less than some X, make him move and perform assistance
	Game::HEntity ActorID = Context.Actors[0];
	if (Context.Actors.size() > 1)
	{
		//...
	}

	// Push standard action for the first actor only
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	return pWorld && PushStandardExecuteAction(*pWorld, ActorID, Context, Enqueue, PushChild);
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
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return;
	auto pLock = pWorld->FindComponent<CLockComponent>(Instance.Targets[0].Entity);
	if (!pLock) return;

	int SkillRollModifier = 0;
	if (auto pStats = pWorld->FindComponent<Sh2::CStatsComponent>(Instance.Actor))
		SkillRollModifier += (pStats->Dexterity - 11); // TODO: utility method Sh2::GetStatModifier(StatValue)!
	if (auto pSkills = pWorld->FindComponent<Sh2::CSkillsComponent>(Instance.Actor))
		SkillRollModifier += pSkills->Lockpicking;
	if (auto pLockpick = pWorld->FindComponent<Sh2::CLockpickComponent>(Instance.Source))
		SkillRollModifier += pLockpick->Modifier;
	// TODO: assistant, if will bother with that

	//!!!FIXME: roll dice (use Session.RNG, call utility method Sh2::SkillCheck(Actor, Lockpicking) or simple Rng.RandomInt(1, 20))!
	int Roll = 10;
	Roll += SkillRollModifier;

	//!!!choose animation!
	//!!!remember difference or result in an ability instance params!
	const int Difference = Roll - pLock->Difficulty;
	if (Difference > 0)
	{
		// success
	}
	else if (Difference > -5)
	{
		// failure
	}
	else
	{
		// jamming
		pLock->Jamming = 5 + Difference;
	}

	// TODO: if target belongs to another faction, create crime stimulus based on character's stealth and lockpicking difficulty

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
