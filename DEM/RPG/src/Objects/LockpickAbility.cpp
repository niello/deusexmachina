#include "LockpickAbility.h"
#include <Objects/LockComponent.h>
#include <Animation/AnimationComponent.h>
#include <Animation/AnimationController.h>
#include <Game/Interaction/AbilityInstance.h>
#include <Game/ECS/GameWorld.h>
#include <Character/StatsComponent.h>
#include <Character/SkillsComponent.h>
#include <Character/StatusEffectLogic.h>
#include <Character/StatusEffect.h>
#include <Items/EquipmentComponent.h>
#include <Items/LockpickComponent.h>
#include <Items/ItemUtils.h>
#include <Objects/OwnedComponent.h>
#include <Math/Math.h>

//!!!FIXME: TMP!
#include <Character/AppearanceComponent.h>

namespace DEM::RPG
{
void RebuildCharacterAppearance(Game::CGameWorld& World, Game::HEntity EntityID, CAppearanceComponent& AppearanceComponent, Resources::CResourceManager& RsrcMgr);

static std::pair<Game::HEntity, int> FindBestLockpick(const Game::CGameWorld& World, Game::HEntity ActorID)
{
	Game::HEntity BestLockpick;
	int BestModifier = std::numeric_limits<int>().min();
	if (auto pEquipment = World.FindComponent<const CEquipmentComponent>(ActorID))
	{
		for (auto [SlotID, SlotType] : pEquipment->Scheme->Slots)
		{
			// TODO: check only certain types of slots or all equipment like now?
			const auto StackID = GetEquippedStack(*pEquipment, SlotID);
			if (auto pTool = FindItemComponent<const Sh2::CLockpickComponent>(World, StackID))
			{
				if (BestLockpick != StackID && BestModifier < pTool->Modifier)
				{
					BestModifier = pTool->Modifier;
					BestLockpick = StackID;
				}
			}
		}

		for (auto StackID : pEquipment->QuickSlots)
		{
			if (auto pTool = FindItemComponent<const Sh2::CLockpickComponent>(World, StackID))
			{
				if (BestLockpick != StackID && BestModifier < pTool->Modifier)
				{
					BestModifier = pTool->Modifier;
					BestLockpick = StackID;
				}
			}
		}
	}

	return { BestLockpick, BestModifier };
}
//---------------------------------------------------------------------

static int GetTotalLockpickingModifier(const Game::CGameWorld& World, Game::HEntity ActorID)
{
	int Total = 0;
	if (auto pStats = World.FindComponent<const Sh2::CStatsComponent>(ActorID))
		Total += (pStats->Dexterity.Get<int>() - 11); // TODO: utility method Sh2::GetStatModifier(StatValue)!
	if (auto pSkills = World.FindComponent<const Sh2::CSkillsComponent>(ActorID))
		Total += pSkills->Lockpicking;

	auto [BestLockpick, BestModifier] = FindBestLockpick(World, ActorID);
	if (BestLockpick) Total += BestModifier;

	return Total;
}
//---------------------------------------------------------------------

CLockpickAbility::CLockpickAbility(std::string_view CursorImage)
{
	_Name = "Lockpick";
	_CursorImage = CursorImage;
}
//---------------------------------------------------------------------

Game::PAbilityInstance CLockpickAbility::CreateInstance(const Game::CInteractionContext& Context) const
{
	return Game::PAbilityInstance(n_new(CSkillCheckAbilityInstance(*this)));
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
		if (auto pStats = pWorld->FindComponent<const Sh2::CStatsComponent>(ActorID))
			if (pStats->CanInteract) return true;

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
	auto pLock = pWorld->FindComponent<const CLockComponent>(Target.Entity);

	// TODO: if jammed, must show an action as disabled with a reason, not skip it
	if (!pLock || pLock->Jamming) return false;

	// Trivial locks can be picked without tools and skill
	// TODO: move constant somewhere (Sh2::Balance::GetValue(ID)?)
	if (pLock->Difficulty < 5) return true;

	// An actor must be able to interact and must have a lockpicking skill opened
	for (auto ActorID : Context.Actors)
	{
		// Must be able to interact with objects
		auto pStats = pWorld->FindComponent<const Sh2::CStatsComponent>(ActorID);
		if (!pStats || !pStats->CanInteract) continue;

		// Must have a Lockpicking skill opened
		if (auto pSkills = pWorld->FindComponent<const Sh2::CSkillsComponent>(ActorID))
		if (!pSkills || pSkills->Lockpicking <= 0) continue;

		// Must have a lockpicking tool equipped or in a quick slot
		if (auto pEquipment = pWorld->FindComponent<const CEquipmentComponent>(ActorID))
		{
			// TODO: check only certain types of slots or all equipment like now?
			for (auto [SlotID, SlotType] : pEquipment->Scheme->Slots)
				if (FindItemComponent<const Sh2::CLockpickComponent>(*pWorld, GetEquippedStack(*pEquipment, SlotID)))
					return true;
			for (auto StackID : pEquipment->QuickSlots)
				if (FindItemComponent<const Sh2::CLockpickComponent>(*pWorld, StackID))
					return true;
		}
	}

	// TODO: must show an action as disabled with a reason, not skip it
	return false;
}
//---------------------------------------------------------------------

bool CLockpickAbility::Execute(Game::CGameSession& Session, Game::CInteractionContext& Context) const
{
	if (Context.Targets.empty() || !Context.Targets[0].Entity || Context.Actors.empty()) return false;

	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;

	Context.Commands.clear();
	Context.Commands.resize(Context.Actors.size()); // NB: intentionally resize, not reserve!

	size_t BestActorIndex = 0;
	int BestModifier = std::numeric_limits<int>().min();
	for (size_t i = 1; i < Context.Actors.size(); ++i)
	{
		const int Modifier = GetTotalLockpickingModifier(*pWorld, Context.Actors[i]);
		if (BestModifier < Modifier)
		{
			BestModifier = Modifier;
			BestActorIndex = i;
		}
	}

	auto& Cmd = Context.Commands[BestActorIndex];
	Cmd = PushStandardExecuteAction(*pWorld, Context.Actors[BestActorIndex], Context);
	return !!Cmd;
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
	auto* pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return;
	auto* pLock = pWorld->FindComponent<CLockComponent>(Instance.Targets[0].Entity);
	if (!pLock) return;

	// TODO: activate trap if there is one triggered by lockpicking!

	const int SkillRollModifier = GetTotalLockpickingModifier(*pWorld, Instance.Actor);

	// TODO: assistant, if will bother with that
	// TODO: if lockpick is breakable, handle it (maybe chance)

	//!!!TODO: to Sh2 balance config!
	constexpr int JammingFailureThreshold = -10;

	// FIXME: use Session.RNG, call utility method Sh2::SkillCheck(Actor, Lockpicking, Source)
	const int Roll = Math::RandomU32(1, 20) + SkillRollModifier;

	Game::CGameVarStorage Vars;
	Vars.Set(CStrID("Skill"), CStrID("Lockpicking"));
	const auto HRoll = Vars.Set(CStrID("Roll"), Roll);
	TriggerStatusEffects(Session, *pWorld, Instance.Actor, CStrID("OnSkillCheck"), Vars, CBhvParamEqOrMissingPolicy{ CStrID("Skill"), CStrID("Lockpicking") });

	// TODO: there also may be forced success or failure (determined by special roll values?).
	const int Difference = Vars.Get(HRoll, Roll) - pLock->Difficulty;
	static_cast<CSkillCheckAbilityInstance&>(Instance).Difference = Difference;

	CStrID AnimAction;
	if (Difference > 0)
	{
		// success
		AnimAction = CStrID("TryOpenDoor"); // FIXME: need correct ID!

		::Sys::Log("***DBG Lockpicking succeeded\n");
	}
	else if (Difference > JammingFailureThreshold)
	{
		// failure
		AnimAction = CStrID("TryOpenDoor"); // FIXME: need correct ID!

		::Sys::Log("***DBG Lockpicking failed\n");
	}
	else
	{
		// jamming
		AnimAction = CStrID("TryOpenDoor"); // FIXME: need correct ID!
		pLock->Jamming = JammingFailureThreshold - Difference + 1; //???here or OnEnd or somewhere OnUpdate?

		::Sys::Log("***DBG Lockpicking failed, lock jammed\n");
	}

	if (auto pAnimComponent = pWorld->FindComponent<Game::CAnimationComponent>(Instance.Actor))
	{
		pAnimComponent->Controller.GetParams().Set<CStrID>(CStrID("Action"), AnimAction);
		SheatheAllItems(*pWorld, Instance.Actor);

		// FIXME: trigger from inside the SheatheAllItems / UnsheatheAllItems
		if (auto* pAppearance = pWorld->FindComponent<CAppearanceComponent>(Instance.Actor))
			RebuildCharacterAppearance(*pWorld, Instance.Actor, *pAppearance, Session.GetResourceManager()); // FIXME: where to get ResourceManager properly?!
	}

	// If this object is owned to other faction, create crime stimulus
	if (auto pOwned = pWorld->FindComponent<const COwnedComponent>(Instance.Targets[0].Entity))
	{
		// TODO:
		//if (pOwned->Owner || pOwned->FactionID != actor faction)
		//  create crime stimulus based on character's stealth and lockpicking difficulty
	}
}
//---------------------------------------------------------------------

AI::ECommandStatus CLockpickAbility::OnUpdate(Game::CGameSession& Session, Game::CAbilityInstance& Instance) const
{
	// TODO: play some sounds

	// TODO:
	// (optional) wait for assistant before start, then perform what's now in OnStart
	// (optional) check if assistant cancelled its action, and if so, remove its bonus

	// TODO: make duration based on skill and dexterity?
	const int Difference = static_cast<CSkillCheckAbilityInstance&>(Instance).Difference;
	float Duration;
	if (Difference > 0) Duration = 3.f;
	else if (Difference > -5) Duration = 1.5f;
	else Duration = 5.f;

	return (Instance.ElapsedTime >= Duration) ? AI::ECommandStatus::Succeeded : AI::ECommandStatus::Running;
}
//---------------------------------------------------------------------

void CLockpickAbility::OnEnd(Game::CGameSession& Session, Game::CAbilityInstance& Instance, AI::ECommandStatus Status) const
{
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return;

	if (auto pAnimComponent = pWorld->FindComponent<Game::CAnimationComponent>(Instance.Actor))
	{
		pAnimComponent->Controller.GetParams().Set<CStrID>(CStrID("Action"), CStrID::Empty);

		//!!!FIXME: need to restore previous state, not just unsheathe everything!
		UnsheatheAllItems(*pWorld, Instance.Actor);

		// FIXME: trigger from inside the SheatheAllItems / UnsheatheAllItems
		if (auto pAppearance = pWorld->FindComponent<CAppearanceComponent>(Instance.Actor))
			RebuildCharacterAppearance(*pWorld, Instance.Actor, *pAppearance, Session.GetResourceManager()); // FIXME: where to get ResourceManager properly?!
	}

	// TODO:
	// play result sound from the lock
	// if character is not in stealth, play result phrase
	// remove crime stimulus

	const int Difference = static_cast<CSkillCheckAbilityInstance&>(Instance).Difference;
	if (Status == AI::ECommandStatus::Succeeded && Difference > 0)
	{
		const auto TargetID = Instance.Targets[0].Entity;
		if (auto* pLock = pWorld->FindComponent<CLockComponent>(TargetID))
		{
			pLock->OnUnlocked(false);
			pWorld->RemoveComponent<CLockComponent>(TargetID);
		}
	}
}
//---------------------------------------------------------------------

}
