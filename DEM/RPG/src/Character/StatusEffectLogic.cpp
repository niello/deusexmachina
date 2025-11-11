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
	// Magnitude of each active instance must be greater than zero. Simply don't use magnitude value in formulas if you need not.
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
	World.ForEachComponent<CStatusEffectsComponent>([&Session, dt](auto EntityID, CStatusEffectsComponent& StatusEffects)
	{
		for (auto& [ID, Stack] : StatusEffects.StatusEffectStacks)
		{
			for (auto& Instance : Stack.Instances)
			{
				const float PrevTime = Instance.Time;
				const float NewTime = PrevTime + dt;

				// gather instances that must tick, by each tick trigger separately
				// apply their magnitudes
				// process tick
				// clear instances expired by time

				//!!!should NOT apply expiration by time (an not only by time?) to instances with lifetime suspended!

				if (NewTime > Instance.Duration) Instance.Magnitude = 0.f;

				//!!!check expiration by signal-less expiration conditions! need flag to identify this case! simply Subs.empty() is not enough,
				//some sub-conditions may have subs, some others don't. Condition must set check-on-update request flag.

				if (Instance.Magnitude <= 0.f)
				{
					// remove instance right now by iterator? or delay for std::remove_if?
					// remember to recalculate stack magnitude and check for stack emptiness
					continue;
				}

				// Time triggers work per instance. Use merge policies to combine instances.
				//
				// for each OnTime trigger
				//   get Delay and Period
				//   check trigger activation with PrevTime & NewTime
				//   check optional condition
				//   call trigger with magnitude of the instance, not of the whole stack
				//
				//???or send instance AND stack magnitudes? what if command will alter a modifier?! E.g. -Magnitude Strength each second!
				//!!!magnitude not always summed up! e.g. two +Strength effects, but system may want to choose a stronger one! StackAccum=Strongest, not Sum.

				// on each trigger stack must walk all its instances and apply them according to its accumulation policies?
				// for time could aggregate all instances that have ticked at this frame, just by the same rule. Aggregation will work.

				// different triggers aggregate differently
				// time trigger need to check prev and new time. Others don't.
				// probably even a condition may depend on a certain instance and can't be checked for the whole stack at once? e.g. conditions on source.
				// at least must filter out instances with suspended behaviour

				//???!!!are trigger params universal?! e.g. character type (source vs target). Initially thought that they will be defined only in conditions.
				//???unify trigger and condition params? Some kind of extended structure:
				// Trigger='X' Condition='Y' Params { ... } Commands [ ... ]

				// "Type": "OnSkillCheck", "SkillType": "Lockpicking"
				// "Type": "OnGainingStatusEffect", "TargetTag": "Poison"

				//!!!trigger reacts on event and checks params!
				// - time trigger checks lifetime before activation (Game)
				// - skill trigger checks a skill ID (RPG)
				// - arbitrary trigger simply fires (Game)
				// so some trigger activation code will be in a game mechanics part, can't simply fire a trigger for a stack, must iterate in place and check specific params!
				// for (trigger : triggers(type))
				//   if custom params match the situation
				//     fire the trigger (gather instances, check optional condition etc)
			}

			// delete expired stacks and send trigger

			//!!!document the problem with adding modifiers from different commands! can be cumulative or overwriting!
			//e.g. OnMagnitudeChange -> set modifier Strength -Mag. Or OnEachHit -> add modifier Strength -Mag. Different types of effects.
		}
	});
}
//---------------------------------------------------------------------

}
