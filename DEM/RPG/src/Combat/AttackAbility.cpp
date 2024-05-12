#include "AttackAbility.h"
#include <Animation/AnimationComponent.h>
#include <Animation/AnimationController.h>
#include <Game/Interaction/AbilityInstance.h>
#include <Game/Interaction/Zone.h>
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/EventsComponent.h>
#include <Character/StatsComponent.h>
#include <Character/SkillsComponent.h>
#include <Combat/DestructibleComponent.h>
#include <Combat/WeaponComponent.h>
#include <Combat/CombatUtils.h>
#include <Items/EquipmentComponent.h>
#include <Items/ItemUtils.h>
#include <Math/Math.h>

//!!!FIXME: TMP!
#include <Character/AppearanceComponent.h>

namespace DEM::RPG
{
static const CStrID sidAction("Action");
static const CStrID sidAttack("Attack");
static const CStrID sidWeaponHands("WeaponHands");

void RebuildCharacterAppearance(Game::CGameWorld& World, Game::HEntity EntityID, CAppearanceComponent& AppearanceComponent, Resources::CResourceManager& RsrcMgr);

CAttackAbility::CAttackAbility(std::string_view CursorImage)
{
	_Name = "Attack";
	_CursorImage = CursorImage;
}
//---------------------------------------------------------------------

Game::PAbilityInstance CAttackAbility::CreateInstance(const Game::CInteractionContext& Context) const
{
	return Game::PAbilityInstance(n_new(CAttackAbilityInstance(*this)));
}
//---------------------------------------------------------------------

bool CAttackAbility::IsAvailable(const Game::CGameSession& Session, const Game::CInteractionContext& Context) const
{
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;

	// Need at least one capable actor
	// TODO: check character caps - to utility function?
	for (auto ActorID : Context.Actors)
		if (auto pStats = pWorld->FindComponent<const Sh2::CStatsComponent>(ActorID))
			if (pStats->Capabilities & Sh2::ECapability::Interact) return true;

	// TODO: also need an attack, either from weapon or natural

	return false;
}
//---------------------------------------------------------------------

inline static bool IsTargetDestructible(const Game::CGameWorld& World, Game::HEntity TargetID)
{
	auto pDestructible = World.FindComponent<const CDestructibleComponent>(TargetID);
	return pDestructible && pDestructible->HP.GetFinalValue() > 0;
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
	return pWorld && IsTargetDestructible(*pWorld, Target.Entity);
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

	bool Result = false;

	for (auto ActorID : Context.Actors)
	{
		if (PushStandardExecuteAction(*pWorld, ActorID, Context, Enqueue, PushChild))
		{
			// TODO: disable sheathing while attacking
			UnsheatheAllItems(*pWorld, ActorID);

			// FIXME: trigger from inside the SheatheAllItems / UnsheatheAllItems
			if (auto pAppearance = pWorld->FindComponent<CAppearanceComponent>(ActorID))
				RebuildCharacterAppearance(*pWorld, ActorID, *pAppearance, Session.GetResourceManager()); // FIXME: where to get ResourceManager properly?!

			Result = true;
		}
	}

	return Result;
}
//---------------------------------------------------------------------

bool CAttackAbility::GetZones(const Game::CGameSession& Session, const Game::CAbilityInstance& Instance, std::vector<const Game::CZone*>& Out) const
{
	// If smart object provided zones, don't add a default one
	if (!Out.empty()) return true;

	//!!!FIXME: need some way to add zones! Store per-ability? May need unique zones. Use strong refs, not raw pointers?
	static Game::CZone Zone(rtm::vector_zero(), 1.f);
	Out.push_back(&Zone);
	return true;
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

	auto& AttackInstance = static_cast<CAttackAbilityInstance&>(Instance);

	if (auto pAnimComponent = pWorld->FindComponent<Game::CAnimationComponent>(Instance.Actor))
	{
		// Find weapon
		const CWeaponComponent* pWeaponComponent = nullptr;
		int Hands = 1;
		if (auto pEquipment = pWorld->FindComponent<const CEquipmentComponent>(Instance.Actor))
		{
			for (size_t HandIdx = 0; HandIdx < pEquipment->Scheme->HandCount; ++HandIdx)
			{
				if (pWeaponComponent = FindItemComponent<const CWeaponComponent>(*pWorld, pEquipment->Hands[HandIdx].ItemStackID))
				{
					if (pWeaponComponent->Big) Hands = 2;
					break;
				}
			}
		}

		// Setup character animation
		pAnimComponent->Controller.SetString(sidAction, sidAttack);
		pAnimComponent->Controller.SetInt(sidWeaponHands, Hands);

		// Calculate damage
		//!!!DBG TMP!
		EDamageType DamageType;
		int Damage = 0;
		if (pWeaponComponent)
		{
			DamageType = pWeaponComponent->Damage.Type;
			for (uint8_t i = 0; i < pWeaponComponent->Damage.x; ++i)
				Damage += Math::RandomU32(1, pWeaponComponent->Damage.y);
			Damage += pWeaponComponent->Damage.z;
		}
		else
		{
			// From bare hands
			DamageType = EDamageType::Bludgeoning;
			Damage = Math::RandomU32(1, 2);
		}

		// Subscribe on Hit event to inflict damage
		if (auto pEvents = pWorld->FindComponent<Game::CEventsComponent>(Instance.Actor))
		{
			//???fallback to animation end event if no Hit is fired during an iteration?
			AttackInstance.HitConn = pEvents->OnEvent.Subscribe(
				[&AttackInstance, pWorld, DamageType, Damage](DEM::Game::HEntity EntityID, CStrID ID, const Data::CParams* pParams, float TimeOffset)
			{
				if (ID == "Hit")
					InflictDamage(*pWorld, AttackInstance.Targets[0].Entity, CStrID::Empty, Damage, DamageType, AttackInstance.Actor);
			});
		}
	}
}
//---------------------------------------------------------------------

Game::EActionStatus CAttackAbility::OnUpdate(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const
{
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return Game::EActionStatus::Failed;

	// TODO: if target is destroyed but action is not cancelled, wait for animation cycle end before returning success?

	return IsTargetDestructible(*pWorld, Instance.Targets[0].Entity) ? Game::EActionStatus::Active : Game::EActionStatus::Succeeded;
}
//---------------------------------------------------------------------

void CAttackAbility::OnEnd(Game::CGameSession& Session, Game::CAbilityInstance& Instance, Game::EActionStatus Status) const
{
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return;

	auto& AttackInstance = static_cast<CAttackAbilityInstance&>(Instance);

	// TODO: restore sheathed state of items? Or switch an actor into cooling down state, so it will become peaceful after timeout and sheathe weapons?

	if (auto pAnimComponent = pWorld->FindComponent<Game::CAnimationComponent>(Instance.Actor))
	{
		pAnimComponent->Controller.SetString(sidAction, CStrID::Empty);
		AttackInstance.HitConn.Disconnect();
	}
}
//---------------------------------------------------------------------

}
