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
bool AddStatusEffect(Game::CGameSession& Session, Game::CGameWorld& World, Game::HEntity TargetID, const CStatusEffectData& Effect, CStatusEffectInstance&& Instance);
void UpdateStatusEffects(Game::CGameSession& Session, Game::CGameWorld& World, float dt);
bool ShouldProcessStatusEffectsInstance(Game::CGameSession& Session, const CStatusEffectInstance& Instance, const CStatusEffectBehaviour& Bhv, const Game::CGameVarStorage& Vars);

HAS_METHOD_WITH_SIGNATURE_TRAIT(ShouldProcessBehaviour);
HAS_METHOD_WITH_SIGNATURE_TRAIT(ShouldProcessInstance);
struct CNoFilterPolicy {};

struct CBhvParamEqOrMissingPolicy
{
	CStrID ParamID;
	CStrID ExpectedValue;

	bool ShouldProcessBehaviour(const CStatusEffectBehaviour& Bhv) const
	{
		const auto Skill = Bhv.Params ? Bhv.Params->Get<CStrID>(ParamID, ExpectedValue) : ExpectedValue;
		return Skill == ExpectedValue;
	}
};

template<typename TPolicy = CNoFilterPolicy>
void TriggerStatusEffect(Game::CGameSession& Session, const CStatusEffectStack& Stack, CStrID Event, Game::CGameVarStorage& Vars, TPolicy Policy = {})
{
	//???!!!store Vars in component? in stack? or even in instance? not to rebuild each time
	//!!!if done, don't forget to remove redundant empty check above!

	//!!!TODO: fill Vars with magnitude etc
	//!!!source and target can be written to Vars, like in Flow! See ResolveEntityID, same as for e.g. conversation Initiator.
	//???get source ID from the first instance? or add only if has a single instance / if is the same in all instances?
	//???TODO: clear Vars from magnitude etc at the end?

	n_assert(!Stack.Instances.empty());

	auto ItBhvs = Stack.pEffectData->Behaviours.find(Event);
	if (ItBhvs == Stack.pEffectData->Behaviours.cend()) return;

	for (const auto& Bhv : ItBhvs->second)
	{
		if constexpr (has_method_with_signature_ShouldProcessBehaviour_v<TPolicy, bool(const CStatusEffectBehaviour&)>)
			if (!Policy.ShouldProcessBehaviour(Bhv)) continue;

		//!!!if bhv condition would be instance-independent, magnitude could be recalculated only when dirty, by the accumulation policy.
		float TotalMagnitude = 0.f;
		for (const auto& Instance : Stack.Instances)
		{
			if (!ShouldProcessStatusEffectsInstance(Session, Instance, Bhv, Vars)) continue;

			if constexpr (has_method_with_signature_ShouldProcessInstance_v<TPolicy, bool(const CStatusEffectInstance&)>)
				if (!Policy.ShouldProcessInstance(Instance)) continue;

			TotalMagnitude += Instance.Magnitude; //???respect magnitude accumulation policy?
		}

		//!!!set TotalMagnitude to context!
		//???handle firing this for each instance separately by magnitude accumulation policy? Sum / Max / Oldest / Newest / Separate; per-bhv.
		if (TotalMagnitude > 0.f)
			Game::ExecuteCommandList(Bhv.Commands, Session, &Vars);
	}
}
//---------------------------------------------------------------------

template<typename TPolicy = CNoFilterPolicy>
void TriggerStatusEffects(Game::CGameSession& Session, Game::CGameWorld& World, Game::HEntity EntityID, CStrID Event, Game::CGameVarStorage& Vars, TPolicy Policy = {})
{
	if (const auto* pStatusEffectComponent = World.FindComponent<const CStatusEffectsComponent>(EntityID))
		for (const auto& [ID, Stack] : pStatusEffectComponent->StatusEffectStacks)
			TriggerStatusEffect(Session, Stack, Event, Vars, Policy);
}
//---------------------------------------------------------------------

}
