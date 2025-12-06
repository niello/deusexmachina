#pragma once
#include <Character/StatusEffect.h>

// ECS systems and utils for character status effect application and management

namespace DEM::Game
{
	class CGameWorld;
	class CGameSession;
}

namespace DEM::RPG
{

bool Command_ApplyStatusEffect(Game::CGameSession& Session, const Data::CParams* pParams, Game::CGameVarStorage* pVars);
bool Command_ModifyStatusEffectMagnitude(Game::CGameSession& Session, const Data::CParams* pParams, Game::CGameVarStorage* pVars);

bool AddStatusEffect(Game::CGameSession& Session, Game::CGameWorld& World, Game::HEntity TargetID, const CStatusEffectData& Effect, PStatusEffectInstance&& Instance);
void UpdateStatusEffects(Game::CGameSession& Session, Game::CGameWorld& World, float dt);
bool ShouldProcessStatusEffectsInstance(Game::CGameSession& Session, const CStatusEffectInstance& Instance, const CStatusEffectBehaviour& Bhv, const Game::CGameVarStorage& Vars);
void ProcessStatusEffectsInstance(Game::CGameSession& Session, float Magnitude, const CStatusEffectBehaviour& Bhv, Game::CGameVarStorage& Vars, float& PendingMagnitude);
std::pair<CStatusEffectStack*, CStatusEffectInstance*> FindCurrentStatusEffectInstance(const Game::CGameSession& Session, const Game::CGameVarStorage& Vars);

HAS_METHOD_WITH_SIGNATURE_TRAIT(ShouldProcessBehaviour);
HAS_METHOD_WITH_SIGNATURE_TRAIT(ShouldProcessInstance);
struct CNoFilterPolicy {};

struct CBhvParamEqOrMissingPolicy
{
	CStrID ParamID;
	CStrID ExpectedValue;

	bool ShouldProcessBehaviour(const CStatusEffectBehaviour& Bhv) const
	{
		return !Bhv.Params || Bhv.Params->Get<CStrID>(ParamID, ExpectedValue) == ExpectedValue;
	}
};

template<typename TPolicy = CNoFilterPolicy>
void TriggerStatusEffect(Game::CGameSession& Session, const CStatusEffectStack& Stack, CStrID Event, Game::CGameVarStorage& Vars, TPolicy Policy = {})
{
	n_assert(!Stack.Instances.empty());
	if (Stack.Instances.empty()) return;

	auto ItBhvs = Stack.pEffectData->Behaviours.find(Event);
	if (ItBhvs == Stack.pEffectData->Behaviours.cend()) return;

	// TODO: method to merge vars from one storage to another in an optimized way, type by type? Or a composite
	// resolver to combine multiple storages? Then could merge pre-baked stack or instance vars into external Vars.

	//!!!source and target can be written to Vars, like in Flow! See ResolveEntityID, same as for e.g. conversation Initiator.
	//???get source ID from the first instance? or add only if has a single instance / if is the same in all instances?
	//!!!for all magnitude policies except sum can determine source etc, because a single Instance is selected!
	Vars.Set(CStrID("StatusEffectID"), Stack.pEffectData->ID);

	for (const auto& Bhv : ItBhvs->second)
	{
		if constexpr (has_method_with_signature_ShouldProcessBehaviour_v<TPolicy, bool(const CStatusEffectBehaviour&)>)
			if (!Policy.ShouldProcessBehaviour(Bhv)) continue;

		//!!!if bhv condition would be instance-independent, magnitude could be recalculated only when dirty, by the accumulation policy.
		float PendingMagnitude = 0.f;
		for (const auto& Instance : Stack.Instances)
		{
			if (!ShouldProcessStatusEffectsInstance(Session, *Instance, Bhv, Vars)) continue;

			if constexpr (has_method_with_signature_ShouldProcessInstance_v<TPolicy, bool(const CStatusEffectInstance&)>)
				if (!Policy.ShouldProcessInstance(*Instance)) continue;

			ProcessStatusEffectsInstance(Session, Instance->Magnitude, Bhv, Vars, PendingMagnitude);
		}

		if (PendingMagnitude > 0.f)
		{
			Vars.Set(CStrID("Magnitude"), PendingMagnitude);
			Game::ExecuteCommandList(Bhv.Commands, Session, &Vars);
		}
	}

	//???TODO: clear Vars from stack and instance vars?
}
//---------------------------------------------------------------------

template<typename TPolicy = CNoFilterPolicy>
void TriggerStatusEffects(Game::CGameSession& Session, Game::CGameWorld& World, Game::HEntity EntityID, CStrID Event, Game::CGameVarStorage& Vars, TPolicy Policy = {})
{
	if (const auto* pStatusEffectComponent = World.FindComponent<const CStatusEffectsComponent>(EntityID))
	{
		Vars.Set(CStrID("StatusEffectOwner"), EntityID);

		for (const auto& [ID, Stack] : pStatusEffectComponent->Stacks)
			TriggerStatusEffect(Session, Stack, Event, Vars, Policy);
	}
}
//---------------------------------------------------------------------

}
