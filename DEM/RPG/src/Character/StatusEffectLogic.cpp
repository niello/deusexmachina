#include "StatusEffectLogic.h"
#include <Character/StatusEffect.h>
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>

namespace DEM::RPG
{

bool Command_ApplyStatusEffect(Game::CGameSession& Session, const Data::CParams* pParams, Game::CGameVarStorage* pVars)
{
	// Params are required because of an effect ID, vars - because of a target entity ID
	if (!pParams || !pVars) return false;

	auto* pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;

	const CStrID ID = pParams->Get<CStrID>(CStrID("ID"), {});
	auto Rsrc = Session.GetResourceManager().RegisterResource<CStatusEffectData>(ID.CStr());
	if (!Rsrc) return false;

	const auto* pEffectData = Rsrc->ValidateObject<CStatusEffectData>();
	if (!pEffectData) return false;

	const auto TargetEntityID = pVars->Get<Game::HEntity>(pVars->Find(CStrID("Target")), {});
	if (!TargetEntityID) return false;

	//!!!TODO: source, tags & target info to an universal command context structure? besides arbitrary pVars.
	//!!!need to take into account expiration conditions, thay will need vars for evaluation!
	CStatusEffectInstance Effect;
	Effect.SourceCreatureID = pVars->Get<Game::HEntity>(pVars->Find(CStrID("SourceCreature")), {});
	Effect.SourceItemStackID = pVars->Get<Game::HEntity>(pVars->Find(CStrID("SourceItemStack")), {});
	Effect.SourceAbilityID = pVars->Get<CStrID>(pVars->Find(CStrID("SourceAbility")), {});
	Effect.SourceStatusEffectID = pVars->Get<CStrID>(pVars->Find(CStrID("SourceStatusEffect")), {});

	Data::PDataArray TagsDesc;
	if (pParams->TryGet(TagsDesc, CStrID("Tags")))
		for (const auto& TagData : *TagsDesc)
			Effect.Tags.insert(TagData.GetValue<CStrID>());

	Data::CData ConditionDesc;
	if (pParams->TryGet(ConditionDesc, CStrID("ExpirationCondition")))
		ParamsFormat::Deserialize(ConditionDesc, Effect.ExpirationCondition);

	Effect.Magnitude = 1.f;
	if (auto* pData = pParams->FindValue(CStrID("Magnitude")))
	{
		if (const auto* pValue = pData->As<float>())
		{
			Effect.Magnitude = *pValue;
		}
		else if (const auto* pValue = pData->As<std::string>())
		{
			//???pass source and target sheets? what else? how to make flexible (easily add other data if needed later)? use environment?
			//???pass here or they must be passed from outside to every command, including this one? commands are also parametrized.
			sol::function Formula = Session.GetScriptState().script("return function(Params, Vars) return {} end"_format(*pValue));
			Effect.Magnitude = Scripting::LuaCall<float>(Formula, pParams, pVars);
		}
	}

	Effect.Duration = STATUS_EFFECT_INFINITE;
	if (auto* pData = pParams->FindValue(CStrID("Duration")))
	{
		if (const auto* pValue = pData->As<float>())
		{
			Effect.Duration = *pValue;
		}
		else if (const auto* pValue = pData->As<std::string>())
		{
			sol::function Formula = Session.GetScriptState().script("return function(Params, Vars) return {} end"_format(*pValue));
			Effect.Duration = Scripting::LuaCall<float>(Formula, pParams, pVars);
		}
	}

	return AddStatusEffect(Session, *pWorld, TargetEntityID, *pEffectData, std::move(Effect));
}
//---------------------------------------------------------------------

bool AddStatusEffect(Game::CGameSession& Session, Game::CGameWorld& World, Game::HEntity TargetID, const CStatusEffectData& Effect, CStatusEffectInstance&& Instance)
{
	// Magnitude must be greater than zero for effect to last. Simply don't use magnitude value in formulas if you need not.
	// Infinite duration is STATUS_EFFECT_INFINITE, other values are treated as an explicit duration.
	if (Instance.Magnitude <= 0.f || Instance.Duration <= 0.f) return false;

	auto* pStatusEffectComponent = World.FindOrAddComponent<CStatusEffectsComponent>(TargetID);
	if (!pStatusEffectComponent) return false;

	// TODO: immunity / blocking
	// check by tags of other status effects if this effect is blocked
	//???where to cache immunity tags? or scan all active stacks and check one by one?
	// Tag immunity (bool or %) is stored in stats and checked here?
	//???are blocking tags and immunity tags the same thing?

	auto [ItStack, IsNew] = pStatusEffectComponent->StatusEffectStacks.try_emplace(Effect.ID);
	auto& Stack = ItStack->second;

	constexpr bool Merge = false;
	if (Merge)
	{
		NOT_IMPLEMENTED;

		// handle policies for duration and magnitude; match sources;
		//!!!???can't ignore source if expiration conditions use it?! requires correct effect data setup from GD?
	}
	else
	{
		// Add a new instance
		Stack.Instances.push_back(std::move(Instance));

		// subscribe on expiration condition events
	}

	if (IsNew)
	{
		// remove effects blocked by us, remember blocking tags in some Tag->Counter cache if needed
		TriggerStatusEffect(Session, Stack, CStrID("OnAdded"), nullptr);
	}

	// clamp magnitude and duration to limits? or apply limits only to the total value in the stack?
	//!!!stack must update totals (magnitude and maybe smth else)!

	return false;
}
//---------------------------------------------------------------------

void TriggerStatusEffects(Game::CGameSession& Session, Game::CGameWorld& World, Game::HEntity EntityID, CStrID Event, Game::CGameVarStorage* pVars)
{
	if (const auto* pStatusEffectComponent = World.FindComponent<const CStatusEffectsComponent>(EntityID))
		for (const auto& [ID, Stack] : pStatusEffectComponent->StatusEffectStacks)
			TriggerStatusEffect(Session, Stack, Event, pVars);
}
//---------------------------------------------------------------------

void TriggerStatusEffect(Game::CGameSession& Session, const CStatusEffectStack& Stack, CStrID Event, Game::CGameVarStorage* pVars)
{
	::Sys::DbgOut("Status effect '{}' trigger '{}'\n"_format(Stack.pEffectData->ID, Event));

	//???lazy refresh stack totals here?
	//???!!!if no pVars, use new temporary ctx? or require pVars to be passed from outside if want to use them?! more optimal.
	//!!!TODO: fill Vars with magnitude etc
	//!!!source and target can be written to Vars, like in Flow! See ResolveEntityID, same as for e.g. conversation Initiator.
	//???get source ID from the first instance? or add only if has a single instance / if is the same in all instances?
	//!!!TODO: clear Vars from magnitude etc
}
//---------------------------------------------------------------------

void UpdateStatusEffects(Game::CGameSession& Session, Game::CGameWorld& World, float dt)
{
	World.ForEachComponent<CStatusEffectsComponent>([&Session](auto EntityID, CStatusEffectsComponent& StatusEffects)
	{
		// ...
	});
}
//---------------------------------------------------------------------

}
