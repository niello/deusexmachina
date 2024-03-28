#include "AttackAbility.h"
#include <Animation/AnimationComponent.h>
#include <Animation/AnimationController.h>
#include <Game/Interaction/AbilityInstance.h>
#include <Game/ECS/GameWorld.h>
#include <Character/StatsComponent.h>
#include <Character/SkillsComponent.h>
#include <Combat/DestructibleComponent.h>
#include <Items/EquipmentComponent.h>
#include <Items/ItemUtils.h>
#include <Math/Math.h>

//!!!FIXME: TMP!
#include <Character/AppearanceComponent.h>

namespace DEM::RPG
{
void RebuildCharacterAppearance(Game::CGameWorld& World, Game::HEntity EntityID, CAppearanceComponent& AppearanceComponent, Resources::CResourceManager& RsrcMgr);

CAttackAbility::CAttackAbility(std::string_view CursorImage)
{
	_Name = "Attack";
	_CursorImage = CursorImage;
}
//---------------------------------------------------------------------

Game::PAbilityInstance CAttackAbility::CreateInstance(const Game::CInteractionContext& Context) const
{
	NOT_IMPLEMENTED;
	return nullptr; // Game::PAbilityInstance(n_new(CSkillCheckAbilityInstance(*this)));
}
//---------------------------------------------------------------------

bool CAttackAbility::IsAvailable(const Game::CGameSession& Session, const Game::CInteractionContext& Context) const
{
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;

	// Need at least one capable actor. No mechanics skill is required because there are locks that are
	// simple enough to be lockpicked by anyone. Hard locks must be filtered in target validation code.
	// TODO: check character caps - to utility function?
	for (auto ActorID : Context.Actors)
		if (auto pStats = pWorld->FindComponent<const Sh2::CStatsComponent>(ActorID))
			if (pStats->Capabilities & Sh2::ECapability::Interact) return true;

	return false;
}
//---------------------------------------------------------------------

bool CAttackAbility::IsTargetValid(const Game::CGameSession& Session, U32 Index, const Game::CInteractionContext& Context) const
{
	// Ability accepts only one target
	if (Index != 0) return false;

	// Check for the destructible component
	const auto& Target = (Index == Context.Targets.size()) ? Context.CandidateTarget : Context.Targets[Index];
	if (!Target.Valid || (!Context.Actors.empty() && Target.Entity == Context.Actors[0])) return false;
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;
	auto pDestructible = pWorld->FindComponent<const CDestructibleComponent>(Target.Entity);
	if (!pDestructible) return false; // TODO: or dead? or in this case destructible component is removed?

	return true;
}
//---------------------------------------------------------------------

ESoftBool CAttackAbility::NeedMoreTargets(const Game::CInteractionContext& Context) const
{
	return (Context.Targets.size() < 1) ? ESoftBool::True : ESoftBool::False;
}
//---------------------------------------------------------------------

bool CAttackAbility::Execute(Game::CGameSession& Session, Game::CInteractionContext& Context, bool Enqueue, bool PushChild) const
{
	if (Context.Targets.empty() || !Context.Targets[0].Entity || Context.Actors.empty()) return false;

	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;

	Game::HEntity BestActorID = Context.Actors[0];

	return PushStandardExecuteAction(*pWorld, BestActorID, Context, Enqueue, PushChild);
}
//---------------------------------------------------------------------

bool CAttackAbility::GetFacingParams(const Game::CGameSession& Session, const Game::CAbilityInstance& Instance, Game::CFacingParams& Out) const
{
	// For now simply face an object origin
	Out.Mode = Game::EFacingMode::Point;
	Out.Dir = vector3::Zero;
	Out.Tolerance = 1.5f;
	return true;
}
//---------------------------------------------------------------------

void CAttackAbility::OnStart(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const
{
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return;

	UnsheatheAllItems(*pWorld, Instance.Actor);

	// FIXME: trigger from inside the SheatheAllItems / UnsheatheAllItems
	if (auto pAppearance = pWorld->FindComponent<CAppearanceComponent>(Instance.Actor))
		RebuildCharacterAppearance(*pWorld, Instance.Actor, *pAppearance, Session.GetResourceManager()); // FIXME: where to get ResourceManager properly?!
}
//---------------------------------------------------------------------

Game::EActionStatus CAttackAbility::OnUpdate(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const
{
	return Game::EActionStatus::Active;
}
//---------------------------------------------------------------------

void CAttackAbility::OnEnd(Game::CGameSession& Session, Game::CAbilityInstance& Instance, Game::EActionStatus Status) const
{
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return;
}
//---------------------------------------------------------------------

}
