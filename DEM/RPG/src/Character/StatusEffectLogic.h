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
void RunStatusEffectBehaviour(Game::CGameSession& Session, const CStatusEffectStack& Stack, const CStatusEffectBehaviour& Bhv, Game::CGameVarStorage& Vars);

HAS_METHOD_WITH_SIGNATURE_TRAIT(ShouldProcessBehaviour);
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
	// Or method like Combine(VarStorage Dest, { VarStorage, ... }) for any number of storages, with optimal reserve. Dest is reused.
	const auto StatusEffectIDHandle = Vars.Set(CStrID("StatusEffectID"), Stack.pEffectData->ID);

	for (const auto& Bhv : ItBhvs->second)
	{
		//???or use only Bhv.Condition?
		if constexpr (has_method_with_signature_ShouldProcessBehaviour_v<TPolicy, bool(const CStatusEffectBehaviour&)>)
			if (!Policy.ShouldProcessBehaviour(Bhv)) continue;

		RunStatusEffectBehaviour(Session, Stack, Bhv, Vars);
	}

	//???TODO: clear Vars from stack and instance vars?
	// TODO: CVarStorage::Erase!
	Vars.Set(StatusEffectIDHandle, CStrID::Empty);
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
