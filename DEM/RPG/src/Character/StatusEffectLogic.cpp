#include "StatusEffectLogic.h"
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

	Effect.RemainingTime = STATUS_EFFECT_INFINITE;
	if (auto* pData = pParams->FindValue(CStrID("Duration")))
	{
		if (const auto* pValue = pData->As<float>())
		{
			Effect.RemainingTime = *pValue;
		}
		else if (const auto* pValue = pData->As<std::string>())
		{
			sol::function Formula = Session.GetScriptState().script("return function(Params, Vars) return {} end"_format(*pValue));
			Effect.RemainingTime = Scripting::LuaCall<float>(Formula, pParams, pVars);
		}
	}

	return AddStatusEffect(Session, *pWorld, TargetEntityID, *pEffectData, std::move(Effect));
}
//---------------------------------------------------------------------

bool AddStatusEffect(Game::CGameSession& Session, Game::CGameWorld& World, Game::HEntity TargetID, const CStatusEffectData& Effect, CStatusEffectInstance&& Instance)
{
	// Magnitude of each active instance must be greater than zero. Simply don't use magnitude value in formulas if you need not.
	// Infinite duration is STATUS_EFFECT_INFINITE, other values are treated as an explicit duration.
	if (Instance.Magnitude <= 0.f || Instance.RemainingTime <= 0.f) return false;

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
		// clamp magnitude and duration to limits? or apply limits only to the total value in the stack?
		//!!!stack must update totals (magnitude and maybe smth else)!
	}
	else
	{
		// Add a new instance
		Stack.Instances.push_back(std::move(Instance));

		// TODO: subscribe on expiration condition events
	}

	if (IsNew)
	{
		Stack.pEffectData = &Effect;

		// TODO: remove effects blocked by us, remember blocking tags in some Tag->Counter cache if needed

		//???!!!store in stack? or even in instance? not to rebuild each time
		Game::CGameVarStorage Vars;
		TriggerStatusEffect(Session, Stack, CStrID("OnAdded"), Vars);
	}

	return false;
}
//---------------------------------------------------------------------

static size_t CalcTimeTriggerActivationCount(float PrevTime, float dt, float Delay, float Period)
{
	const float NewTime = PrevTime + dt;
	const bool WaitFirstTick = ((!PrevTime && !Delay) || PrevTime < Delay);

	if (Period <= 0.f)
		return WaitFirstTick && Delay <= NewTime; // will return 1 or 0

	float NextTickTime = WaitFirstTick ? Delay : (PrevTime + Period - std::fmodf(PrevTime - Delay, Period));
	size_t TickCount = 0;
	while (NextTickTime <= NewTime)
	{
		++TickCount;
		NextTickTime += Period;
	}
	return TickCount;
}
//---------------------------------------------------------------------

void UpdateStatusEffects(Game::CGameSession& Session, Game::CGameWorld& World, float dt)
{
	// Tick logic requires dt > 0, or the same tick might happen multiple times
	if (dt <= 0.f) return;

	World.ForEachComponent<CStatusEffectsComponent>([&Session, dt](auto EntityID, CStatusEffectsComponent& StatusEffects)
	{
		//???!!!store in stack? or even in instance? not to rebuild each time
		Game::CGameVarStorage Vars;

		for (auto& [ID, Stack] : StatusEffects.StatusEffectStacks)
		{
			// Check instance expiration from conditions
			for (auto& Instance : Stack.Instances)
			{
				//!!!check expiration by signal-less expiration conditions! need flag out arg in SubscribeRelevantEvents to identify this case!
				// simply Subs.empty() is not enough, some sub-conditions may have subs, some others don't. Condition must set check-on-update request flag.
				//???or, if this flag is set, subscriptions are of no use and can be cleared? Or subscribe not always for recalc?
				//Instance.Magnitude = 0.f;

				//???erase here instead of erase-remove? don't evaluate condition if magnitude is already <= 0.f!
			}

			// Remove instances expired by magnitude before further processing
			Stack.Instances.erase(std::remove_if(Stack.Instances.begin(), Stack.Instances.end(), [](const auto& Instance) { return Instance.Magnitude <= 0.f; }), Stack.Instances.end());

			if (!Stack.Instances.empty())
			{
				// Trigger OnTime behaviours on this stack
				auto ItBhvs = Stack.pEffectData->Behaviours.find(CStrID("OnTime"));
				if (ItBhvs != Stack.pEffectData->Behaviours.cend())
				{
					for (const auto& Bhv : ItBhvs->second)
					{
						// Can't process OnTime trigger without params
						n_assert(Bhv.Params);
						if (!Bhv.Params) continue;

						const float Delay = Bhv.Params->Get<float>(CStrID("Delay"), 0.f);
						const float Period = Bhv.Params->Get<float>(CStrID("Period"), 0.f);

						float PendingMagnitude = 0.f;
						for (const auto& Instance : Stack.Instances)
						{
							if (!ShouldProcessStatusEffectsInstance(Session, Instance, Bhv, Vars)) continue;

							const auto ActivationCount = CalcTimeTriggerActivationCount(Instance.Time, dt, Delay, Period);
							for (size_t i = 0; i < ActivationCount; ++i)
								ProcessStatusEffectsInstance(Session, Instance.Magnitude, Bhv, Vars, PendingMagnitude);
						}

						if (PendingMagnitude > 0.f)
						{
							//!!!set PendingMagnitude to context!
							Game::ExecuteCommandList(Bhv.Commands, Session, &Vars);
						}
					}
				}

				// Advance instance time and remove instances expired by time
				for (auto It = Stack.Instances.begin(); It != Stack.Instances.end(); /**/)
				{
					if (!It->SuspendLifetimeCounter)
					{
						if (It->RemainingTime <= dt)
						{
							It = Stack.Instances.erase(It);
							continue;
						}

						It->RemainingTime -= dt;
					}

					It->Time += dt;
					++It;
				}
			}

			if (Stack.Instances.empty())
			{
				TriggerStatusEffect(Session, Stack, CStrID("OnRemoved"), Vars);

				// delete expired stack
			}

			//!!!document the problem with adding modifiers from different commands! can be cumulative or overwriting!
			//e.g. OnMagnitudeChange -> set modifier Strength -Mag. Or OnEachHit -> add modifier Strength -Mag. Different types of effects.
		}
	});
}
//---------------------------------------------------------------------

bool ShouldProcessStatusEffectsInstance(Game::CGameSession& Session, const CStatusEffectInstance& Instance,
	const CStatusEffectBehaviour& Bhv, const Game::CGameVarStorage& Vars)
{
	return !Instance.SuspendBehaviourCounter && Instance.Magnitude > 0.f && Game::EvaluateCondition(Bhv.Condition, Session, &Vars);
}
//---------------------------------------------------------------------

void ProcessStatusEffectsInstance(Game::CGameSession& Session, float Magnitude, const CStatusEffectBehaviour& Bhv,
	Game::CGameVarStorage& Vars, float& PendingMagnitude)
{
	switch (Bhv.MagnitudePolicy)
	{
		case EStatusEffectMagnitudePolicy::Sum:
		{
			PendingMagnitude += Magnitude;
			break;
		}
		case EStatusEffectMagnitudePolicy::Max:
		{
			if (PendingMagnitude < Magnitude)
				PendingMagnitude = Magnitude;
			break;
		}
		case EStatusEffectMagnitudePolicy::Oldest:
		{
			if (PendingMagnitude <= 0.f)
				PendingMagnitude = Magnitude;
			break;
		}
		case EStatusEffectMagnitudePolicy::Newest:
		{
			PendingMagnitude = Magnitude;
			break;
		}
		case EStatusEffectMagnitudePolicy::Separate:
		{
			//!!!TODO: set magnitude from instance!
			Game::ExecuteCommandList(Bhv.Commands, Session, &Vars);
			break;
		}
	}
}
//---------------------------------------------------------------------

}
