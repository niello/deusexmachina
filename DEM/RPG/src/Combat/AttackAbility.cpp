#include "AttackAbility.h"
#include <Animation/AnimationComponent.h>
#include <Animation/AnimationController.h>
#include <Game/Interaction/AbilityInstance.h>
#include <Game/Interaction/Zone.h>
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/EventsComponent.h>
#include <Scripting/LogicRegistry.h>
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
		if (auto* pStats = pWorld->FindComponent<const Sh2::CStatsComponent>(ActorID))
			if (pStats->CanInteract) return true;

	// TODO: also need an attack, either from weapon or natural

	return false;
}
//---------------------------------------------------------------------

inline static bool IsTargetDestructible(const Game::CGameWorld& World, Game::HEntity TargetID)
{
	auto* pDestructible = World.FindComponent<const CDestructibleComponent>(TargetID);
	return pDestructible && pDestructible->HP > 0;
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

bool CAttackAbility::Execute(Game::CGameSession& Session, Game::CInteractionContext& Context) const
{
	if (Context.Targets.empty() || !Context.Targets[0].Entity || Context.Actors.empty()) return false;

	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;

	Context.Commands.clear();
	Context.Commands.reserve(Context.Actors.size());

	bool Result = false;

	for (auto ActorID : Context.Actors)
	{
		Context.Commands.push_back(PushStandardExecuteAction(*pWorld, ActorID, Context));

		if (Context.Commands.back())
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

static const CWeaponComponent* FindCurrentWeapon(Game::CGameWorld& World, Game::HEntity ActorID)
{
	if (auto* pEquipment = World.FindComponent<const CEquipmentComponent>(ActorID))
		for (size_t HandIdx = 0; HandIdx < pEquipment->Scheme->HandCount; ++HandIdx)
			if (auto* pWeaponComponent = FindItemComponent<const CWeaponComponent>(World, pEquipment->Hands[HandIdx].ItemStackID))
				return pWeaponComponent;

	return nullptr;
}
//---------------------------------------------------------------------

static void InitStrike(Game::CGameWorld& World, CAttackAbilityInstance& Instance)
{
	// Find weapon
	const auto* pWeaponComponent = FindCurrentWeapon(World, Instance.Actor);
	const int Hands = (pWeaponComponent && pWeaponComponent->Big) ? 2 : 1;

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
		pAnimComponent->Controller.GetParams().Set<CStrID>(sidAction, sidAttack);
		pAnimComponent->Controller.GetParams().Set<int>(sidWeaponHands, Hands);
		//pAnimComponent->Controller.SetBool(sidIsInCombat, true);

		// Apply AC param changes to receive correct animation length
		//!!!TODO: check that when updating for dt=0, it must not trigger (and therefore skip) anim event on which it currently stands, if any!!!
		pAnimComponent->Controller.Update(pAnimComponent->Output, 0.f, nullptr);

		// Avoid slowing animation down too much
		constexpr float MinSpeedMul = 0.75f;

		const float AnimLength = pAnimComponent->Controller.GetExpectedAnimationLength();
		pAnimComponent->Controller.GetParams().Set<float>(sidActionSpeedMul, std::max(MinSpeedMul, AnimLength / Period));
	}
}
//---------------------------------------------------------------------

static void ApplyDamageFromAbility(Game::CGameSession& Session, Game::CGameWorld& World, CAttackAbilityInstance& Instance)
{
	if (Instance.DamageType == EDamageType::COUNT) return;

	InflictDamage(World, Instance.Targets[0].Entity, Instance.Location, Instance.Damage, Instance.DamageType, Instance.Actor);

	if (const auto* pGameLogic = Session.FindFeature<Game::CLogicRegistry>())
	{
		//???don't calculate damage in advance? only check hit/miss type?
		//???apply OnHit commands from the natural weapon if no pWeaponComponent? or must find natiral as pWeaponComponent?!!!
		if (const auto* pWeaponComponent = FindCurrentWeapon(World, Instance.Actor))
		{
			// apply OnHit commands from this weapon

			//!!!DBG TMP!
			if (auto* pCmd = pGameLogic->FindCommand(CStrID("DealDamage")))
			{
				(*pCmd)(Session, nullptr, nullptr);
			}
			else
			{
				// log command not found
			}
		}

		//!!!apply OnHit commands from all active status effects!
		//???no special commands from ability here? need special abilities to cast additional effects?
	}

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
			[&AttackInstance, &Session, pWorld](DEM::Game::HEntity EntityID, CStrID ID, const Data::CParams* pParams, float TimeOffset)
		{
			if (ID == "Hit")
			{
				// Logically strike a target at the proper moment of attack animation to sync with visuals
				ApplyDamageFromAbility(Session, *pWorld, AttackInstance);
			}
			else if (ID == Anim::Event_AnimEnd)
			{
				//!!!FIXME: need to check what animation was finished, maybe it was e.g. hit reaction clip!
				//???maybe should cancel strike here? then any animation will do it, but only Hit animation will inflict damage.
				//???but what to do with attack period? Strike anim end doesn't cancel waiting, other anim restarts strike from beginning!
				//!!!need to handle interruptions (e.g. from being hit) here, attack action must be cancelled or at least reset!
				ApplyDamageFromAbility(Session, *pWorld, AttackInstance);
				if (auto pAnimComponent = pWorld->FindComponent<Game::CAnimationComponent>(AttackInstance.Actor))
				{
					pAnimComponent->Controller.GetParams().Set<CStrID>(sidAction, CStrID::Empty);
					pAnimComponent->Controller.GetParams().Set<float>(sidActionSpeedMul, 1.f);
				}
			}
		});
	}
}
//---------------------------------------------------------------------

AI::ECommandStatus CAttackAbility::OnUpdate(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const
{
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return AI::ECommandStatus::Failed;

	auto& AttackInstance = static_cast<CAttackAbilityInstance&>(Instance);

	// If the strike time reached and damage wasn't yet inflicted, must do it now
	if (AttackInstance.DamageType != EDamageType::COUNT && AttackInstance.ElapsedTime >= AttackInstance.StrikeEndTime)
		ApplyDamageFromAbility(Session, *pWorld, AttackInstance);

	// Could wait for the end of the attack Period here, but now we chose to succeed immediately, allowing an actor to proceed with the next action
	if (!IsTargetDestructible(*pWorld, Instance.Targets[0].Entity)) return AI::ECommandStatus::Succeeded;

	// If the previous strike is finished, start new one
	if (AttackInstance.DamageType == EDamageType::COUNT && AttackInstance.ElapsedTime >= AttackInstance.StrikeEndTime)
		InitStrike(*pWorld, AttackInstance);

	return AI::ECommandStatus::Running;
}
//---------------------------------------------------------------------

void CAttackAbility::OnEnd(Game::CGameSession& Session, Game::CAbilityInstance& Instance, AI::ECommandStatus Status) const
{
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return;

	auto& AttackInstance = static_cast<CAttackAbilityInstance&>(Instance);

	// TODO: restore sheathed state of items? Or switch an actor into cooling down state, so it will become peaceful after timeout and sheathe weapons?

	if (auto pAnimComponent = pWorld->FindComponent<Game::CAnimationComponent>(Instance.Actor))
	{
		pAnimComponent->Controller.GetParams().Set<CStrID>(sidAction, CStrID::Empty);
		pAnimComponent->Controller.GetParams().Set<float>(sidActionSpeedMul, 1.f);
	}

	AttackInstance.AnimEventConn.Disconnect();
}
//---------------------------------------------------------------------

}
