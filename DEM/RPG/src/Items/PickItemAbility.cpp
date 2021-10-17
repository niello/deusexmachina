#include "PickItemAbility.h"
//#include <Animation/AnimationComponent.h>
//#include <Animation/AnimationController.h>
#include <Game/Interaction/AbilityInstance.h>
#include <Game/Interaction/Zone.h>
#include <Game/ECS/GameWorld.h>
#include <Physics/RigidBodyComponent.h>
#include <Character/StatsComponent.h>
//#include <Character/SkillsComponent.h>
#include <Items/ItemStackComponent.h>
#include <Scene/SceneComponent.h>
#include <Items/ItemUtils.h>
//#include <Objects/OwnedComponent.h>
//#include <Math/Math.h>

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
		if (auto pStats = pWorld->FindComponent<Sh2::CStatsComponent>(ActorID))
			if (pStats->Capabilities & Sh2::ECapability::Interact) return true;

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
	return pWorld && pWorld->FindComponent<CItemStackComponent>(Target.Entity);
}
//---------------------------------------------------------------------

ESoftBool CPickItemAbility::NeedMoreTargets(const Game::CInteractionContext& Context) const
{
	return (Context.Targets.size() < 1) ? ESoftBool::True : ESoftBool::False;
}
//---------------------------------------------------------------------

bool CPickItemAbility::Execute(Game::CGameSession& Session, Game::CInteractionContext& Context, bool Enqueue, bool PushChild) const
{
	if (Context.Targets.empty() || !Context.Targets[0].Entity || Context.Actors.empty()) return false;

	Game::HEntity ActorID = Context.Actors[0];
	// TODO: choose the first actor with enough inventory space? Or no need?
	//if (Context.Actors.size() > 1)
	//{
	//}

	// Push standard action for the first actor only
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	return pWorld && PushStandardExecuteAction(*pWorld, ActorID, Context, Enqueue, PushChild);
}
//---------------------------------------------------------------------

bool CPickItemAbility::GetZones(const Game::CGameSession& Session, const Game::CAbilityInstance& Instance, std::vector<const Game::CZone*>& Out) const
{
	// If smart object provided zones, don't add a default one
	if (!Out.empty()) return true;

	//!!!FIXME: need some way to add zones! Store per-ability? May need unique zones. Use strong refs, not raw pointers?
	static Game::CZone Zone(vector3::Zero, 1.f);
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
}
//---------------------------------------------------------------------

Game::EActionStatus CPickItemAbility::OnUpdate(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const
{
	return Game::EActionStatus::Succeeded;
}
//---------------------------------------------------------------------

void CPickItemAbility::OnEnd(Game::CGameSession& Session, Game::CAbilityInstance& Instance, Game::EActionStatus Status) const
{
	if (Status != Game::EActionStatus::Succeeded) return;

	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return;

	// TODO: if owned by another faction, create crime stimulus/signal based on Steal skill check,
	// it may even interrupt item picking (in OnStart()?)

	// TODO: check character inventory free weight and volume
	// TODO: merge with existing stack if possible
	// TODO: equip if a) default equipping makes sense, like for weapons b) inventory is full but equipment slot isn't
	// NB: equipped things ignore volume limitations, but not weight

	AddItemsIntoContainer(*pWorld, Instance.Actor, Instance.Targets[0].Entity);
}
//---------------------------------------------------------------------

}
