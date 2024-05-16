#include "AttackAbility.h"
#include <Animation/AnimationComponent.h>
#include <Animation/AnimationController.h>
#include <Game/Interaction/AbilityInstance.h>
#include <Game/Interaction/Zone.h>
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/EventsComponent.h>
#include <AI/Navigation/NavAgentComponent.h>
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
static const CStrID sidActionSpeedMul("ActionSpeedMul");

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

	// Fallback to a navigation agent radius, this is a sane default for creatures
	//!!!FIXME: storing raw pointers in Out doesn't allow to add zones on the fly!
	/*
	if (auto pWorld = Session.FindFeature<Game::CGameWorld>())
	{
		if (auto pAgent = pWorld->FindComponent<const AI::CNavAgentComponent>(Instance.Actor))
		{
			Game::CZone NavAgentZone(rtm::vector_zero(), pAgent->Radius);
			Out.push_back(&NavAgentZone);
			return true;
		}
	}
	*/

	// The last resort
	//???need? or let it fail?
	static const Game::CZone Zone(rtm::vector_zero(), 1.f);
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

static void InitStrike(Game::CGameWorld& World, CAttackAbilityInstance& Instance)
{
	// Find weapon
	const CWeaponComponent* pWeaponComponent = nullptr;
	int Hands = 1;
	if (auto pEquipment = World.FindComponent<const CEquipmentComponent>(Instance.Actor))
	{
		for (size_t HandIdx = 0; HandIdx < pEquipment->Scheme->HandCount; ++HandIdx)
		{
			if (pWeaponComponent = FindItemComponent<const CWeaponComponent>(World, pEquipment->Hands[HandIdx].ItemStackID))
			{
				if (pWeaponComponent->Big) Hands = 2;
				break;
			}
		}
	}

	// Calculate strike params
	//!!!DBG TMP!
	float Period;
	if (pWeaponComponent)
	{
		Instance.DamageType = pWeaponComponent->Damage.Type;
		Instance.Damage = pWeaponComponent->Damage.z;
		for (uint8_t i = 0; i < pWeaponComponent->Damage.x; ++i)
			Instance.Damage += Math::RandomU32(1, pWeaponComponent->Damage.y);
		Period = pWeaponComponent->Period;
	}
	else
	{
		// From bare hands
		// FIXME: need to calc from actor stats
		Instance.DamageType = EDamageType::Bludgeoning;
		Instance.Damage = Math::RandomU32(1, 2);
		Period = 2.0f;
	}

	// FIXME: need better handling for this case? Assert?
	if (Period <= 0.f) Period = 0.001f;

	// TODO: determine location, also can setup AC params, especially IK, to animate strike to that location visually
	Instance.Location = {};

	Instance.StrikeEndTime = Instance.ElapsedTime + Period;

	// Setup character animation
	if (auto pAnimComponent = World.FindComponent<Game::CAnimationComponent>(Instance.Actor))
	{
		pAnimComponent->Controller.SetString(sidAction, sidAttack);
		pAnimComponent->Controller.SetInt(sidWeaponHands, Hands);
		//pAnimComponent->Controller.SetBool(sidIsInCombat, true);

		// Apply AC param changes to receive correct animation length
		//!!!TODO: check that when updating for dt=0, it must not trigger (and therefore skip) anim event on which it currently stands, if any!!!
		pAnimComponent->Controller.Update(pAnimComponent->Output, 0.f, nullptr);

		// Avoid slowing animation down too much
		constexpr float MinSpeedMul = 0.75f;

		const float AnimLength = pAnimComponent->Controller.GetExpectedAnimationLength();
		pAnimComponent->Controller.SetFloat(sidActionSpeedMul, std::max(MinSpeedMul, AnimLength / Period));
	}
}
//---------------------------------------------------------------------

static void ApplyDamageFromAbility(Game::CGameWorld& World, CAttackAbilityInstance& Instance)
{
	if (Instance.DamageType == EDamageType::COUNT) return;

	InflictDamage(World, Instance.Targets[0].Entity, Instance.Location, Instance.Damage, Instance.DamageType, Instance.Actor);

	Instance.DamageType = EDamageType::COUNT; // Reset applied damage
}
//---------------------------------------------------------------------

void CAttackAbility::OnStart(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const
{
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return;

	auto& AttackInstance = static_cast<CAttackAbilityInstance&>(Instance);

	InitStrike(*pWorld, AttackInstance);

	// Subscribe on Hit event to inflict damage and to animation end event to switch to combat idle
	if (auto pEvents = pWorld->FindComponent<Game::CEventsComponent>(Instance.Actor))
	{
		AttackInstance.AnimEventConn = pEvents->OnEvent.Subscribe(
			[&AttackInstance, pWorld](DEM::Game::HEntity EntityID, CStrID ID, const Data::CParams* pParams, float TimeOffset)
		{
			if (ID == "Hit")
			{
				// Logically strike a target at the proper moment of attack animation to sync with visuals
				ApplyDamageFromAbility(*pWorld, AttackInstance);
			}
			else if (ID == Anim::Event_AnimEnd)
			{
				//!!!FIXME: need to check what animation was finished, maybe it was e.g. hit reaction clip!
				//???maybe should cancel strike here? then any animation will do it, but only Hit animation will inflict damage.
				//???but what to do with attack period? Strike anim end doesn't cancel waiting, other anim restarts strike from beginning!
				//!!!need to handle interruptions (e.g. from being hit) here, attack action must be cancelled or at least reset!
				ApplyDamageFromAbility(*pWorld, AttackInstance);
				if (auto pAnimComponent = pWorld->FindComponent<Game::CAnimationComponent>(AttackInstance.Actor))
				{
					pAnimComponent->Controller.SetString(sidAction, CStrID::Empty);
					pAnimComponent->Controller.SetFloat(sidActionSpeedMul, 1.f);
				}
			}
		});
	}
}
//---------------------------------------------------------------------

Game::EActionStatus CAttackAbility::OnUpdate(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const
{
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return Game::EActionStatus::Failed;

	auto& AttackInstance = static_cast<CAttackAbilityInstance&>(Instance);

	// If the strike time reached and damage wasn't yet inflicted, must do it now
	if (AttackInstance.DamageType != EDamageType::COUNT && AttackInstance.ElapsedTime >= AttackInstance.StrikeEndTime)
		ApplyDamageFromAbility(*pWorld, AttackInstance);

	// Could wait for the end of the attack Period here, but now we chose to succeed immediately, allowing an actor to proceed with the next action
	if (!IsTargetDestructible(*pWorld, Instance.Targets[0].Entity)) return Game::EActionStatus::Succeeded;

	// If the previous strike is finished, start new one
	if (AttackInstance.DamageType == EDamageType::COUNT && AttackInstance.ElapsedTime >= AttackInstance.StrikeEndTime)
		InitStrike(*pWorld, AttackInstance);

	return Game::EActionStatus::Active;
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
		pAnimComponent->Controller.SetFloat(sidActionSpeedMul, 1.f);
	}

	AttackInstance.AnimEventConn.Disconnect();
}
//---------------------------------------------------------------------

}
