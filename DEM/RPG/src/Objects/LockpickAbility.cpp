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

	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;

	if (auto pQueue = pWorld->FindComponent<Game::CActionQueueComponent>(Context.Actors[0]))
	{
		Game::PAbilityInstance AbilityInstance(n_new(Game::CAbilityInstance(*this)));
		AbilityInstance->Source = Context.Source;
		AbilityInstance->Targets = Context.Targets;

		// FIXME: CODE DUPLICATION AMONG ALL ABILITIES!
		if (PushChild && pQueue->GetCurrent()) pQueue->PushOrUpdateChild<Game::ExecuteAbility>(pQueue->GetCurrent(), std::move(AbilityInstance));
		else
		{
			if (!Enqueue) pQueue->Reset();
			pQueue->EnqueueAction<Game::ExecuteAbility>(std::move(AbilityInstance));
		}
		return true;
	}

	return false;
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
	if (auto pWorld = Session.FindFeature<Game::CGameWorld>())
		pWorld->RemoveComponent<CLockComponent>(Instance.Targets[0].Entity);
}
//---------------------------------------------------------------------

}
