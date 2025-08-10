#include "PickItemAbility.h"
#include <Game/Interaction/AbilityInstance.h>
#include <Game/Interaction/Zone.h>
#include <Game/ECS/GameWorld.h>
#include <Character/StatsComponent.h>
#include <Items/ItemComponent.h> // currency
#include <Items/ItemStackComponent.h>
#include <Items/ItemUtils.h>

namespace DEM::RPG
{

CPickItemAbility::CPickItemAbility(std::string_view CursorImage)
{
	_Name = "PickItem";
	_CursorImage = CursorImage;
}
//---------------------------------------------------------------------

bool CPickItemAbility::IsAvailable(const Game::CGameSession& Session, const Game::CInteractionContext& Context) const
{
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;

	// Need at least one capable actor with enough inventory space
	// TODO: check space!
	for (auto ActorID : Context.Actors)
		if (auto pStats = pWorld->FindComponent<const Sh2::CStatsComponent>(ActorID))
			if (pStats->CanInteract) return true;

	return false;
}
//---------------------------------------------------------------------

bool CPickItemAbility::IsTargetValid(const Game::CGameSession& Session, U32 Index, const Game::CInteractionContext& Context) const
{
	// Ability accepts only one target
	if (Index != 0) return false;

	// Check for the item stack component
	const auto& Target = (Index == Context.Targets.size()) ? Context.CandidateTarget : Context.Targets[Index];
	if (!Target.Valid) return false;
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	return pWorld && pWorld->FindComponent<const CItemStackComponent>(Target.Entity);
}
//---------------------------------------------------------------------

bool CPickItemAbility::Execute(Game::CGameSession& Session, Game::CInteractionContext& Context) const
{
	if (Context.Targets.empty() || !Context.Targets[0].Entity || Context.Actors.empty()) return false;

	// Push standard action for the first actor only
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;

	Context.Commands.clear();
	Context.Commands.resize(Context.Actors.size()); // NB: intentionally resize, not reserve!

	Game::HEntity ActorID = Context.Actors[0];

	size_t BestActorIndex = 0;
	// TODO: choose the first actor with enough inventory space? Or the closest one? Or no need?
	//if (Context.Actors.size() > 1)
	//{
	//}

	auto& Cmd = Context.Commands[BestActorIndex];
	Cmd = PushStandardExecuteAction(*pWorld, Context.Actors[BestActorIndex], Context);
	return !!Cmd;
}
//---------------------------------------------------------------------

bool CPickItemAbility::GetZones(const Game::CGameSession& Session, const Game::CAbilityInstance& Instance, std::vector<const Game::CZone*>& Out) const
{
	// If smart object provided zones, don't add a default one
	if (!Out.empty()) return true;

	//!!!FIXME: need some way to add zones! Store per-ability? May need unique zones. Use strong refs, not raw pointers?
	static Game::CZone Zone(rtm::vector_zero(), 1.f);
	Out.push_back(&Zone);
	return true;
}
//---------------------------------------------------------------------

bool CPickItemAbility::GetFacingParams(const Game::CGameSession& Session, const Game::CAbilityInstance& Instance, Game::CFacingParams& Out) const
{
	// Simply face an object origin
	//???make this logic default???
	Out.Mode = Game::EFacingMode::Point;
	Out.Dir = vector3::Zero;
	Out.Tolerance = 1.5f;
	return true;
}
//---------------------------------------------------------------------

void CPickItemAbility::OnStart(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const
{
	// TODO: animation?
}
//---------------------------------------------------------------------

AI::ECommandStatus CPickItemAbility::OnUpdate(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const
{
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return AI::ECommandStatus::Failed;

	const auto StackID = Instance.Targets[0].Entity;

	// TODO: if owned by another faction, create crime stimulus/signal based on Steal skill check,
	// it may even interrupt item picking (in OnStart()?)

	// TODO: equip if a) default equipping makes sense, like for weapons b) inventory is full but equipment slot isn't
	// NB: equipped things ignore volume limitations, but not weight

	if (TryPickCurrency(Session, Instance.Actor, StackID))
	{
		RemoveItemVisualsFromLocation(*pWorld, StackID);
		return AI::ECommandStatus::Succeeded;
	}

	const auto [AddedCount, MovedCompletely] = MoveItemsToContainer(*pWorld, Instance.Actor, StackID);
	if (MovedCompletely)
		RemoveItemVisualsFromLocation(*pWorld, StackID);

	return AddedCount ? AI::ECommandStatus::Succeeded : AI::ECommandStatus::Failed;
}
//---------------------------------------------------------------------

void CPickItemAbility::OnEnd(Game::CGameSession& Session, Game::CAbilityInstance& Instance, AI::ECommandStatus Status) const
{
	// TODO: animation?
}
//---------------------------------------------------------------------

}
