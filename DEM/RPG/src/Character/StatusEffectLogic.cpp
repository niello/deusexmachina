#include "StatusEffectLogic.h"
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Scripting/LogicRegistry.h>

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
	PStatusEffectInstance Instance(new CStatusEffectInstance());
	Instance->SourceCreatureID = pVars->Get<Game::HEntity>(pVars->Find(CStrID("SourceCreature")), {});
	Instance->SourceItemStackID = pVars->Get<Game::HEntity>(pVars->Find(CStrID("SourceItemStack")), {});
	Instance->SourceAbilityID = pVars->Get<CStrID>(pVars->Find(CStrID("SourceAbility")), {});
	Instance->SourceStatusEffectID = pVars->Get<CStrID>(pVars->Find(CStrID("SourceStatusEffect")), {});

	Data::PDataArray TagsDesc;
	if (pParams->TryGet(TagsDesc, CStrID("Tags")))
		for (const auto& TagData : *TagsDesc)
			Instance->Tags.insert(TagData.GetValue<CStrID>());

	Data::CData ConditionDesc;
	if (pParams->TryGet(ConditionDesc, CStrID("ValidityCondition")))
		ParamsFormat::Deserialize(ConditionDesc, Instance->ValidityCondition);

	Instance->Magnitude = 1.f;
	if (auto* pData = pParams->FindValue(CStrID("Magnitude")))
	{
		if (const auto* pValue = pData->As<float>())
		{
			Instance->Magnitude = *pValue;
		}
		else if (const auto* pValue = pData->As<std::string>())
		{
			//???pass source and target sheets? what else? how to make flexible (easily add other data if needed later)? use environment?
			//???pass here or they must be passed from outside to every command, including this one? commands are also parametrized.
			sol::function Formula = Session.GetScriptState().script("return function(Params, Vars) return {} end"_format(*pValue));
			Instance->Magnitude = Scripting::LuaCall<float>(Formula, pParams, pVars);
		}
	}

	Instance->RemainingTime = STATUS_EFFECT_INFINITE;
	if (auto* pData = pParams->FindValue(CStrID("Duration")))
	{
		if (const auto* pValue = pData->As<float>())
		{
			Instance->RemainingTime = *pValue;
		}
		else if (const auto* pValue = pData->As<std::string>())
		{
			sol::function Formula = Session.GetScriptState().script("return function(Params, Vars) return {} end"_format(*pValue));
			Instance->RemainingTime = Scripting::LuaCall<float>(Formula, pParams, pVars);
		}
	}

	return AddStatusEffect(Session, *pWorld, TargetEntityID, *pEffectData, std::move(Instance));
}
//---------------------------------------------------------------------

static bool MergeStatusEffectInstances(const CStatusEffectData& Effect, CStatusEffectInstance& Dest, CStatusEffectInstance& Src)
{
	if (Effect.SourceMergePolicy == EStatusEffectSetMergePolicy::FullMatch)
	{
		if (Dest.SourceCreatureID != Src.SourceCreatureID ||
			Dest.SourceItemStackID != Src.SourceItemStackID ||
			Dest.SourceAbilityID != Src.SourceAbilityID ||
			Dest.SourceStatusEffectID != Src.SourceStatusEffectID)
		{
			return false;
		}
	}

	if (Effect.TagMergePolicy == EStatusEffectSetMergePolicy::FullMatch && Dest.Tags != Src.Tags)
		return false;

	switch (Effect.SourceMergePolicy)
	{
		case EStatusEffectSetMergePolicy::All:
		{
			// Not exactly "All" but somewhat close
			if (!Dest.SourceCreatureID) Dest.SourceCreatureID = Src.SourceCreatureID;
			if (!Dest.SourceItemStackID) Dest.SourceItemStackID = Src.SourceItemStackID;
			if (!Dest.SourceAbilityID) Dest.SourceAbilityID = Src.SourceAbilityID;
			if (!Dest.SourceStatusEffectID) Dest.SourceStatusEffectID = Src.SourceStatusEffectID;
			break;
		}
		case EStatusEffectSetMergePolicy::Matching:
		{
			if (Dest.SourceCreatureID != Src.SourceCreatureID) Dest.SourceCreatureID = {};
			if (Dest.SourceItemStackID != Src.SourceItemStackID) Dest.SourceItemStackID = {};
			if (Dest.SourceAbilityID != Src.SourceAbilityID) Dest.SourceAbilityID = {};
			if (Dest.SourceStatusEffectID != Src.SourceStatusEffectID) Dest.SourceStatusEffectID = {};
			break;
		}
		case EStatusEffectSetMergePolicy::Last:
		{
			Dest.SourceCreatureID = Src.SourceCreatureID;
			Dest.SourceItemStackID = Src.SourceItemStackID;
			Dest.SourceAbilityID = Src.SourceAbilityID;
			Dest.SourceStatusEffectID = Src.SourceStatusEffectID;
			break;
		}
	}

	switch (Effect.TagMergePolicy)
	{
		case EStatusEffectSetMergePolicy::All:
		{
			Dest.Tags.merge(std::move(Src.Tags));
			break;
		}
		case EStatusEffectSetMergePolicy::Matching:
		{
			Algo::InplaceIntersection(Dest.Tags, Src.Tags);
			break;
		}
		case EStatusEffectSetMergePolicy::Last:
		{
			Dest.Tags = std::move(Src.Tags);
			break;
		}
	}

	switch (Effect.MagnitudeMergePolicy)
	{
		case EStatusEffectNumMergePolicy::Sum:
		{
			Dest.Magnitude += Src.Magnitude;
			break;
		}
		case EStatusEffectNumMergePolicy::Max:
		{
			if (Dest.Magnitude < Src.Magnitude)
				Dest.Magnitude = Src.Magnitude;
			break;
		}
		case EStatusEffectNumMergePolicy::Last:
		{
			Dest.Magnitude = Src.Magnitude;
			break;
		}
	}

	switch (Effect.DurationMergePolicy)
	{
		case EStatusEffectNumMergePolicy::Sum:
		{
			Dest.RemainingTime += Src.RemainingTime;
			break;
		}
		case EStatusEffectNumMergePolicy::Max:
		{
			if (Dest.RemainingTime < Src.RemainingTime)
				Dest.RemainingTime = Src.RemainingTime;
			break;
		}
		case EStatusEffectNumMergePolicy::Last:
		{
			Dest.RemainingTime = Src.RemainingTime;
			break;
		}
	}

	return true;
}
//---------------------------------------------------------------------

bool AddStatusEffect(Game::CGameSession& Session, Game::CGameWorld& World, Game::HEntity TargetID, const CStatusEffectData& Effect, PStatusEffectInstance&& Instance)
{
	// Magnitude of each active instance must be greater than zero. Simply don't use magnitude value in formulas if you need not.
	// Infinite duration is STATUS_EFFECT_INFINITE, other values are treated as an explicit duration.
	if (Instance->Magnitude <= 0.f || Instance->RemainingTime <= 0.f) return false;

	auto* pStatusEffectComponent = World.FindOrAddComponent<CStatusEffectsComponent>(TargetID);
	if (!pStatusEffectComponent) return false;

	//???!!!store Vars in stack? or even in instance? not to rebuild each time
	Game::CGameVarStorage Vars;
	Vars.Set(CStrID("SourceCreature"), Instance->SourceCreatureID);
	Vars.Set(CStrID("SourceItemStack"), Instance->SourceItemStackID);
	if (!Game::EvaluateCondition(Instance->ValidityCondition, Session, &Vars)) return false;

	// TODO: immunity / blocking
	//!!!NB: blocking and suspension can use instance tags before it is merged!
	// check by tags of other status effects if this effect is blocked
	//???where to cache immunity tags? or scan all active stacks and check one by one?
	// Tag immunity (bool or %) is stored in stats and checked here?
	//???are blocking tags and immunity tags the same thing?

	auto [ItStack, IsNewStack] = pStatusEffectComponent->StatusEffectStacks.try_emplace(Effect.ID);
	auto& Stack = ItStack->second;

	bool Merged = false;
	if (Effect.Merge)
	{
		for (auto& ExistingInstance : Stack.Instances)
		{
			if (MergeStatusEffectInstances(Effect, *ExistingInstance, *Instance))
			{
				Merged = true;
				break;
			}
		}
	}

	if (!Merged)
	{
		// Choose between existing and the new instance if the stacking policy requires it
		switch (Effect.StackPolicy)
		{
			case EStatusEffectStackPolicy::Discard:
			{
				if (!Stack.Instances.empty()) return false;
				break;
			}
			case EStatusEffectStackPolicy::Replace:
			{
				Stack.Instances.clear();
				break;
			}
			case EStatusEffectStackPolicy::KeepLongest:
			{
				for (const auto& ExistingInstance : Stack.Instances)
					if (ExistingInstance->RemainingTime > Instance->RemainingTime)
						return false;

				Stack.Instances.clear();
				break;
			}
			case EStatusEffectStackPolicy::KeepStrongest:
			{
				for (const auto& ExistingInstance : Stack.Instances)
					if (ExistingInstance->Magnitude > Instance->Magnitude)
						return false;

				Stack.Instances.clear();
				break;
			}
		}

		// Clear magnitude when expiration condition is met. This effectively invalidates an instance.
		if (const auto* pConditions = Session.FindFeature<Game::CLogicRegistry>())
		{
			if (auto* pCondition = pConditions->FindCondition(Instance->ValidityCondition.Type))
			{
				pCondition->SubscribeRelevantEvents(Instance->ConditionSubs, { Instance->ValidityCondition, Session, &Vars },
					[pInstance = Instance.get(), &Session](std::unique_ptr<Game::CGameVarStorage>& EventVars)
				{
					if (!EventVars) EventVars = std::make_unique<Game::CGameVarStorage>();
					EventVars->Set(CStrID("SourceCreature"), pInstance->SourceCreatureID);
					EventVars->Set(CStrID("SourceItemStack"), pInstance->SourceItemStackID);

					if (!Game::EvaluateCondition(pInstance->ValidityCondition, Session, EventVars.get()))
						pInstance->Magnitude = 0.f;
				});
			}
		}

		// Add a new instance
		Stack.Instances.push_back(std::move(Instance));
	}

	if (IsNewStack)
	{
		Stack.pEffectData = &Effect;

		// TODO: remove effects blocked by us, remember blocking tags in some Tag->Counter cache if needed

		TriggerStatusEffect(Session, Stack, CStrID("OnAdded"), Vars);
	}

	return true;
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
		//???!!!store Vars in stack? or even in instance? not to rebuild each time
		Game::CGameVarStorage Vars;

		for (auto ItStack = StatusEffects.StatusEffectStacks.begin(); ItStack != StatusEffects.StatusEffectStacks.end(); /**/)
		{
			auto& [ID, Stack] = *ItStack;

			// Remove instances expired by conditions or magnitude before further processing.
			// Must check condition that has no subscriptions each frame. If a part of a condition
			// can't rely on signals, all subs are dropped.
			for (auto ItInstance = Stack.Instances.begin(); ItInstance != Stack.Instances.end(); /**/)
			{
				const auto* pInstance = (*ItInstance).get();
				if (pInstance->Magnitude <= 0.f)
					ItInstance = Stack.Instances.erase(ItInstance);
				else if (pInstance->ConditionSubs.empty() && !Game::EvaluateCondition(pInstance->ValidityCondition, Session, &Vars))
					ItInstance = Stack.Instances.erase(ItInstance);
				else
					++ItInstance;
			}

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
							if (!ShouldProcessStatusEffectsInstance(Session, *Instance, Bhv, Vars)) continue;

							const auto ActivationCount = CalcTimeTriggerActivationCount(Instance->Time, dt, Delay, Period);
							for (size_t i = 0; i < ActivationCount; ++i)
								ProcessStatusEffectsInstance(Session, Instance->Magnitude, Bhv, Vars, PendingMagnitude);
						}

						if (PendingMagnitude > 0.f)
						{
							Vars.Set(CStrID("Magnitude"), PendingMagnitude);
							Game::ExecuteCommandList(Bhv.Commands, Session, &Vars);
						}
					}
				}

				// Advance instance time and remove instances expired by time
				for (auto ItInstance = Stack.Instances.begin(); ItInstance != Stack.Instances.end(); /**/)
				{
					auto* pInstance = (*ItInstance).get();
					if (!pInstance->SuspendLifetimeCounter)
					{
						if (pInstance->RemainingTime <= dt)
						{
							ItInstance = Stack.Instances.erase(ItInstance);
							continue;
						}

						pInstance->RemainingTime -= dt;
					}

					pInstance->Time += dt;
					++ItInstance;
				}
			}

			// Remove totally expired status effect stacks
			if (Stack.Instances.empty())
			{
				// OnRemoved behaviour is instance-less, handle manually
				auto ItBhvs = Stack.pEffectData->Behaviours.find(CStrID("OnRemoved"));
				if (ItBhvs != Stack.pEffectData->Behaviours.cend())
				{
					//!!!source and target can be written to Vars, like in Flow! See ResolveEntityID, same as for e.g. conversation Initiator.
					//???get source ID from the first instance? or add only if has a single instance / if is the same in all instances?
					//!!!for all magnitude policies except sum can determine source etc, because a single Instance is selected!
					Vars.Set(CStrID("StatusEffectID"), Stack.pEffectData->ID);

					for (const auto& Bhv : ItBhvs->second)
						if (Game::EvaluateCondition(Bhv.Condition, Session, &Vars))
							Game::ExecuteCommandList(Bhv.Commands, Session, &Vars);

					//???TODO: clear Vars from stack vars?
				}

				ItStack = StatusEffects.StatusEffectStacks.erase(ItStack);
				continue;
			}

			++ItStack;
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
	if (!Bhv.MagnitudeAggregation)
	{
		Vars.Set(CStrID("Magnitude"), Magnitude);
		Game::ExecuteCommandList(Bhv.Commands, Session, &Vars);
		return;
	}

	switch (*Bhv.MagnitudeAggregation)
	{
		case EStatusEffectNumMergePolicy::Sum:
		{
			PendingMagnitude += Magnitude;
			break;
		}
		case EStatusEffectNumMergePolicy::Max:
		{
			if (PendingMagnitude < Magnitude)
				PendingMagnitude = Magnitude;
			break;
		}
		case EStatusEffectNumMergePolicy::First:
		{
			if (PendingMagnitude <= 0.f)
				PendingMagnitude = Magnitude;
			break;
		}
		case EStatusEffectNumMergePolicy::Last:
		{
			PendingMagnitude = Magnitude;
			break;
		}
	}
}
//---------------------------------------------------------------------

}
