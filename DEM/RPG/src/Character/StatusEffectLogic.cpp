#include "StatusEffectLogic.h"
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Character/StatsComponent.h>
#include <Scripting/LogicRegistry.h>

namespace DEM::RPG
{
static void OnAggregatedMagnitudeChanged(Game::CGameSession& Session, CStatusEffectStack& Stack, float NewMagnitude);
static void OnSeparateMagnitudeChanged(Game::CGameSession& Session, const CStatusEffectData& Effect, CStatusEffectInstance& Instance, float PrevValue);

static void MergeValues(float& Dest, float Src, EStatusEffectNumMergePolicy Policy)
{
	switch (Policy)
	{
		case EStatusEffectNumMergePolicy::Sum:
		{
			Dest += Src;
			break;
		}
		case EStatusEffectNumMergePolicy::Max:
		{
			if (Dest < Src)
				Dest = Src;
			break;
		}
		case EStatusEffectNumMergePolicy::Last:
		{
			Dest = Src;
			break;
		}
	}
}
//---------------------------------------------------------------------

static inline bool IsInstanceActive(const CStatusEffectInstance& Instance)
{
	return !Instance.SuspendBehaviourCounter && Instance.Magnitude > 0.f;
}
//---------------------------------------------------------------------

static float CalcAggregatedMagnitude(const CStatusEffectStack& Stack)
{
	float Magnitude = 0.f;
	for (const auto& Instance : Stack.Instances)
	{
		if (!IsInstanceActive(*Instance)) continue;

		if (Stack.pEffectData->MagnitudeAggregationPolicy == EStatusEffectNumMergePolicy::First)
		{
			Magnitude = Instance->Magnitude;
			break;
		}

		MergeValues(Magnitude, Instance->Magnitude, Stack.pEffectData->MagnitudeAggregationPolicy);
	}

	return std::min(Magnitude, Stack.pEffectData->MaxAggregatedMagnitude);
}
//---------------------------------------------------------------------

static void RunBhvAggregated(Game::CGameSession& Session, CStatusEffectStack& Stack,
	const CStatusEffectBehaviour& Bhv, Game::CGameVarStorage& Vars, float Magnitude)
{
	if (Magnitude <= 0.f) return;

	// TODO: add merged source and tags info? All aggregations except Sum choose one active instance!

	// TODO: pass and modify duration the same way? at least for separate?
	const auto MagnitudeHandle = Vars.Set(CStrID("Magnitude"), Magnitude);
	Game::ExecuteCommandList(Bhv.Commands, Session, &Vars);
	const float NewMagnitude = std::clamp(Vars.Get<float>(MagnitudeHandle, 0.f), 0.f, Stack.pEffectData->MaxAggregatedMagnitude);

	if (NewMagnitude != Magnitude)
	{
		if (NewMagnitude > Magnitude)
		{
			// Add to the newest active instance
			for (auto It = Stack.Instances.rbegin(); It != Stack.Instances.rend(); ++It)
			{
				auto& Instance = **It;
				if (IsInstanceActive(Instance))
				{
					Instance.Magnitude += (NewMagnitude - Magnitude);
					break;
				}
			}
		}
		else
		{
			// Subtract from oldest first
			float RemainingDiff = Magnitude - NewMagnitude;
			for (auto& Instance : Stack.Instances)
			{
				if (!IsInstanceActive(*Instance)) continue;

				if (Instance->Magnitude < RemainingDiff)
				{
					RemainingDiff -= Instance->Magnitude;
					Instance->Magnitude = 0.f;
				}
				else
				{
					Instance->Magnitude -= RemainingDiff;
					RemainingDiff = 0.f;
					break;
				}
			}

			// Can't happen because Magnitude is aggregated from active instances and NewMagnitude can't be < 0
			n_assert(RemainingDiff == 0.f);
		}

		OnAggregatedMagnitudeChanged(Session, Stack, NewMagnitude);
	}
}
//---------------------------------------------------------------------

static void RunBhvSeparate(Game::CGameSession& Session, const CStatusEffectData& Effect, CStatusEffectInstance& Instance, const CStatusEffectBehaviour& Bhv, Game::CGameVarStorage& Vars)
{
	// Some behaviours may need to run on suspended or expired instances
	const float Magnitude = Instance.SuspendBehaviourCounter ? 0.f : Instance.Magnitude;

	// TODO: add source and tags info?

	// TODO: pass and modify duration the same way?
	const auto MagnitudeHandle = Vars.Set(CStrID("Magnitude"), Magnitude);
	Game::ExecuteCommandList(Bhv.Commands, Session, &Vars);
	const float NewMagnitude = std::max(Vars.Get<float>(MagnitudeHandle, 0.f), 0.f);

	if (NewMagnitude != Magnitude)
	{
		// Changing magnitude of a suspended instance is always a sign of an incorrect effect setup.
		// Commands see Magnitude 0 and can't resume suspended instances by altering it.
		n_assert(!Instance.SuspendBehaviourCounter);

		Instance.Magnitude = NewMagnitude;
		OnSeparateMagnitudeChanged(Session, Effect, Instance, Magnitude);
	}
}
//---------------------------------------------------------------------

static void OnAggregatedMagnitudeChanged(Game::CGameSession& Session, CStatusEffectStack& Stack, float NewMagnitude)
{
	const float PrevMagnitude = Stack.AggregatedMagnitude;
	if (PrevMagnitude == NewMagnitude) return;
	Stack.AggregatedMagnitude = NewMagnitude;

	//!!!update Stack modifiers based on magnitude (re-eval formulas), erase mods that became +0 or *1!

	auto ItBhvs = Stack.pEffectData->Behaviours.find(CStrID("OnMagnitudeChanged"));
	if (ItBhvs != Stack.pEffectData->Behaviours.cend())
	{
		Game::CGameVarStorage Vars;
		//Vars.Set(CStrID("StatusEffectOwner"), EntityID);
		Vars.Set(CStrID("StatusEffectID"), Stack.pEffectData->ID);
		Vars.Set(CStrID("PrevMagnitude"), PrevMagnitude);

		// NB: must not use NewMagnitude here, because an aggregated magnitude can change in subsequent commands
		for (const auto& Bhv : ItBhvs->second)
			if (Game::EvaluateCondition(Bhv.Condition, Session, &Vars))
				RunBhvAggregated(Session, Stack, Bhv, Vars, Stack.AggregatedMagnitude);
	}
}
//---------------------------------------------------------------------

static void OnSeparateMagnitudeChanged(Game::CGameSession& Session, const CStatusEffectData& Effect, CStatusEffectInstance& Instance, float PrevValue)
{
	const float NewValue = Instance.SuspendBehaviourCounter ? 0.f : Instance.Magnitude;
	if (PrevValue == NewValue) return;

	//!!!update Stack modifiers based on magnitude (re-eval formulas), erase mods that became +0 or *1!

	auto ItBhvs = Effect.Behaviours.find(CStrID("OnMagnitudeChanged"));
	if (ItBhvs != Effect.Behaviours.cend())
	{
		Game::CGameVarStorage Vars;
		//Vars.Set(CStrID("StatusEffectOwner"), EntityID);
		Vars.Set(CStrID("StatusEffectID"), Effect.ID);
		Vars.Set(CStrID("PrevMagnitude"), PrevValue);
		for (const auto& Bhv : ItBhvs->second)
			if (Game::EvaluateCondition(Bhv.Condition, Session, &Vars))
				RunBhvSeparate(Session, Effect, Instance, Bhv, Vars);
	}
}
//---------------------------------------------------------------------

static inline void OnMagnitudeChanged(Game::CGameSession& Session, CStatusEffectStack& Stack, CStatusEffectInstance& Instance, float PrevValue)
{
	// Aggregated effects don't handle individual instance magnitude changes, only an aggregated value matters
	if (Stack.pEffectData->Aggregated)
		OnAggregatedMagnitudeChanged(Session, Stack, CalcAggregatedMagnitude(Stack));
	else
		OnSeparateMagnitudeChanged(Session, *Stack.pEffectData, Instance, PrevValue);
}
//---------------------------------------------------------------------

static float EvaluateCommandNumericValue(Game::CGameSession& Session, const Data::CParams* pParams, Game::CGameVarStorage* pVars, CStrID ID, float Default)
{
	if (auto* pData = pParams ? pParams->FindValue(ID) : nullptr)
	{
		if (const auto* pValue = pData->As<float>())
			return *pValue;

		if (const auto* pValue = pData->As<int>())
			return static_cast<float>(*pValue);

		if (const auto* pValue = pData->As<std::string>())
		{
			//???pass source and target sheets? what else? how to make flexible (easily add other data if needed later)?
			//???use environment instead of an immediate call of a temporary function?
			//???pass here or they must be passed from outside to every command, including this one? commands are also parametrized.
			//!!!can be even Vars.Get(Params.MyID)!
			//!!!also can call a function, even bound from C++! Formula can be like DEM.RPG.LinearStepwisePoison(Vars.Get('Magnitude')).
			sol::function Formula = Session.GetScriptState().script("return function(Params, Vars) return {} end"_format(*pValue));
			return Scripting::LuaCall<float>(Formula, pParams, pVars);
		}
	}

	return Default;
}
//---------------------------------------------------------------------

template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
static T EvaluateCommandEnumValue(Game::CGameSession& Session, const Data::CParams* pParams, CStrID ID, T Default)
{
	T Value = Default;
	Data::CData Data;
	if (pParams && pParams->TryGet(Data, ID))
		ParamsFormat::Deserialize(Data, Value);
	return Value;
}
//---------------------------------------------------------------------

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

	PStatusEffectInstance Instance(new CStatusEffectInstance());
	Instance->SourceCreatureID = pVars->Get<Game::HEntity>(pVars->Find(CStrID("SourceCreature")), {});
	Instance->SourceItemStackID = pVars->Get<Game::HEntity>(pVars->Find(CStrID("SourceItemStack")), {});
	Instance->SourceAbilityID = pVars->Get<CStrID>(pVars->Find(CStrID("SourceAbility")), {});
	Instance->SourceStatusEffectID = pVars->Get<CStrID>(pVars->Find(CStrID("SourceStatusEffect")), {});

	Data::CData TagsDesc;
	if (pParams->TryGet(TagsDesc, CStrID("Tags")))
		ParamsFormat::Deserialize(TagsDesc, Instance->Tags);

	Data::CData ConditionDesc;
	if (pParams->TryGet(ConditionDesc, CStrID("ValidityCondition")))
		ParamsFormat::Deserialize(ConditionDesc, Instance->ValidityCondition);

	Instance->Magnitude = EvaluateCommandNumericValue(Session, pParams, pVars, CStrID("Magnitude"), 1.f);
	Instance->RemainingTime = EvaluateCommandNumericValue(Session, pParams, pVars, CStrID("Duration"), STATUS_EFFECT_INFINITE);

	// NB: positive magnitude check is inside
	return AddStatusEffect(Session, *pWorld, TargetEntityID, *pEffectData, std::move(Instance));
}
//---------------------------------------------------------------------

bool Command_ModifyStatusEffectMagnitude(Game::CGameSession& Session, const Data::CParams* pParams, Game::CGameVarStorage* pVars)
{
	if (!pVars) return false;

	const auto MagnitudeHandle = pVars->Find(CStrID("Magnitude"));
	if (!MagnitudeHandle || !pVars->IsA<float>(MagnitudeHandle)) return false;

	float Magnitude = pVars->Get<float>(MagnitudeHandle);

	const auto Amount = EvaluateCommandNumericValue(Session, pParams, pVars, CStrID("Amount"), 1.f);

	enum class EOp { Mul, Add, Set };
	const EOp Op = EvaluateCommandEnumValue(Session, pParams, CStrID("Op"), EOp::Add);
	switch (Op)
	{
		case EOp::Mul: Magnitude *= Amount; break;
		case EOp::Add: Magnitude += Amount; break;
		case EOp::Set: Magnitude = Amount; break;
	}

	pVars->Set(MagnitudeHandle, Magnitude);

	return true;
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

	MergeValues(Dest.RemainingTime, Src.RemainingTime, Effect.DurationMergePolicy);
	MergeValues(Dest.Magnitude, Src.Magnitude, Effect.MagnitudeMergePolicy);

	return true;
}
//---------------------------------------------------------------------

static void AddSuspensionFromTag(CStatusEffectInstance& Instance, CStrID Tag, const CStatusEffectData& Effect, const CStatusEffectsComponent& Component)
{
	auto It = Component.SuspendedTags.find(Tag);
	if (It == Component.SuspendedTags.cend()) return;

	if (It->second.Behaviour)
	{
		Instance.SuspendBehaviourCounter += It->second.Behaviour;
		if (Effect.SuspendBehaviourTags.find(Tag) != Effect.SuspendBehaviourTags.cend())
			--Instance.SuspendBehaviourCounter;
	}

	if (It->second.Lifetime)
	{
		Instance.SuspendLifetimeCounter += It->second.Lifetime;
		if (Effect.SuspendLifetimeTags.find(Tag) != Effect.SuspendLifetimeTags.cend())
			--Instance.SuspendLifetimeCounter;
	}
}
//---------------------------------------------------------------------

static void ToggleSuspensionFromEffect(Game::CGameSession& Session, CStatusEffectsComponent& Component, const CStatusEffectData& Effect, bool Enable)
{
	const int32_t Mod = Enable ? 1 : -1;

	for (const CStrID Tag : Effect.SuspendBehaviourTags)
	{
		Component.SuspendedTags[Tag].Behaviour += Mod;

		for (auto& [ID, Stack] : Component.Stacks)
		{
			// An effect doesn't suspend itself
			if (Effect.ID == ID) continue;

			bool AggregatedMagnitudeChanged = false;
			const bool HasTag = (Stack.pEffectData->Tags.find(Tag) != Stack.pEffectData->Tags.cend());
			for (auto& Instance : Stack.Instances)
			{
				if (!HasTag && (Instance->Tags.find(Tag) == Instance->Tags.cend())) continue;

				const bool WasSuspended = Instance->SuspendBehaviourCounter;
				Instance->SuspendBehaviourCounter += Mod;
				const bool IsSuspended = Instance->SuspendBehaviourCounter;

				// Suspended instance magnitude is effectively 0, must treat suspension state changes as magnitude changes
				if (WasSuspended != IsSuspended && Instance->Magnitude > 0.f)
				{
					if (Stack.pEffectData->Aggregated)
						AggregatedMagnitudeChanged = true;
					else
						OnSeparateMagnitudeChanged(Session, *Stack.pEffectData, *Instance, WasSuspended ? 0.f : Instance->Magnitude);
				}
			}

			if (AggregatedMagnitudeChanged)
				OnAggregatedMagnitudeChanged(Session, Stack, CalcAggregatedMagnitude(Stack));
		}
	}

	for (const CStrID Tag : Effect.SuspendLifetimeTags)
	{
		Component.SuspendedTags[Tag].Lifetime += Mod;

		for (auto& [ID, Stack] : Component.Stacks)
		{
			// Always called before the stack is added or after it is removed
			n_assert_dbg(Effect.ID != ID);

			const bool HasTag = (Stack.pEffectData->Tags.find(Tag) != Stack.pEffectData->Tags.cend());
			for (auto& Instance : Stack.Instances)
				if (HasTag || (Instance->Tags.find(Tag) != Instance->Tags.cend()))
					Instance->SuspendLifetimeCounter += Mod;
		}
	}
}
//---------------------------------------------------------------------

bool AddStatusEffect(Game::CGameSession& Session, Game::CGameWorld& World, Game::HEntity TargetID, const CStatusEffectData& Effect, PStatusEffectInstance&& Instance)
{
	// Magnitude of each active instance must be greater than zero. Simply don't use magnitude value in formulas if you need not.
	// Infinite duration is STATUS_EFFECT_INFINITE, other values are treated as an explicit duration.
	if (Instance->Magnitude <= 0.f || Instance->RemainingTime <= 0.f) return false;

	auto* pStatusEffectComponent = World.FindOrAddComponent<CStatusEffectsComponent>(TargetID);
	if (!pStatusEffectComponent) return false;

	// Remove additional instance tags that duplicate common effect tags
	Algo::InplaceDifference(Instance->Tags, Effect.Tags);

	// Check for tag-based immunities
	if (auto* pStats = World.FindComponent<const Sh2::CStatsComponent>(TargetID))
	{
		for (const CStrID Tag : Effect.Tags)
		{
			auto It = pStats->TagImmunity.find(Tag);
			if (It != pStats->TagImmunity.cend() && It->second.Get())
				return false;
		}
		for (const CStrID Tag : Instance->Tags)
		{
			auto It = pStats->TagImmunity.find(Tag);
			if (It != pStats->TagImmunity.cend() && It->second.Get())
				return false;
		}
	}

	//???!!!store Vars in stack? or even in instance? not to rebuild each time
	Game::CGameVarStorage Vars;
	Vars.Set(CStrID("StatusEffectOwner"), TargetID);
	Vars.Set(CStrID("SourceCreature"), Instance->SourceCreatureID);
	Vars.Set(CStrID("SourceItemStack"), Instance->SourceItemStackID);
	if (!Game::EvaluateCondition(Instance->ValidityCondition, Session, &Vars)) return false;

	auto [ItStack, IsNewStack] = pStatusEffectComponent->Stacks.try_emplace(Effect.ID);
	auto& Stack = ItStack->second;

	n_assert_dbg(IsNewStack == Stack.Instances.empty());

	if (IsNewStack)
	{
		Stack.pEffectData = &Effect;

		// NB: a new stack is not in the list yet, and it is intentional
		ToggleSuspensionFromEffect(Session, *pStatusEffectComponent, Effect, true);
	}
	else
	{
		// Try merging a new instance into an existing one if allowed
		if (Effect.AllowMerge)
		{
			for (auto& ExistingInstance : Stack.Instances)
			{
				const float PrevMagnitude = ExistingInstance->Magnitude;
				if (MergeStatusEffectInstances(Effect, *ExistingInstance, *Instance))
				{
					if (!ExistingInstance->SuspendBehaviourCounter)
						OnMagnitudeChanged(Session, Stack, *ExistingInstance, PrevMagnitude);
					return true;
				}
			}
		}

		// Choose between existing and the new instance if the stacking policy requires it
		switch (Effect.StackPolicy)
		{
			// case EStatusEffectStackPolicy::Stack: do nothing, just add a new instance
			case EStatusEffectStackPolicy::Discard:
			{
				return false;
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
	}

	// Init suspension counters for a new instance according to currently suspended tags
	for (const CStrID Tag : Effect.Tags)
		AddSuspensionFromTag(*Instance, Tag, Effect, *pStatusEffectComponent);
	for (const CStrID Tag : Instance->Tags)
		AddSuspensionFromTag(*Instance, Tag, Effect, *pStatusEffectComponent);

	// Zero out magnitude when expiration condition is met. This effectively invalidates an instance.
	if (const auto* pConditions = Session.FindFeature<Game::CLogicRegistry>())
	{
		if (auto* pCondition = pConditions->FindCondition(Instance->ValidityCondition.Type))
		{
			pCondition->SubscribeRelevantEvents(Instance->ConditionSubs, { Instance->ValidityCondition, Session, &Vars },
				[pStatusEffectComponent, pInstance = Instance.get(), &Session, EffectID = Effect.ID](std::unique_ptr<Game::CGameVarStorage>& EventVars)
			{
				// Skip already expired instances
				if (pInstance->Magnitude <= 0.f) return;

				if (!EventVars) EventVars = std::make_unique<Game::CGameVarStorage>();
				EventVars->Set(CStrID("SourceCreature"), pInstance->SourceCreatureID);
				EventVars->Set(CStrID("SourceItemStack"), pInstance->SourceItemStackID);

				if (!Game::EvaluateCondition(pInstance->ValidityCondition, Session, EventVars.get()))
				{
					const float PrevMagnitude = std::exchange(pInstance->Magnitude, 0.f);

					if (!pInstance->SuspendBehaviourCounter)
					{
						auto It = pStatusEffectComponent->Stacks.find(EffectID);
						if (It != pStatusEffectComponent->Stacks.cend())
							OnMagnitudeChanged(Session, It->second, *pInstance, pInstance->Magnitude);
					}
				}
			});
		}
	}

	// Add a new instance
	Stack.Instances.push_back(std::move(Instance));

	if (IsNewStack)
		TriggerStatusEffect(Session, Stack, CStrID("OnAdded"), Vars);

	auto& NewInstance = *Stack.Instances.back();
	if (!NewInstance.SuspendBehaviourCounter)
		OnMagnitudeChanged(Session, Stack, NewInstance, 0.f);

	return true;
}
//---------------------------------------------------------------------

void RunStatusEffectBehaviour(Game::CGameSession& Session, CStatusEffectStack& Stack, const CStatusEffectBehaviour& Bhv, Game::CGameVarStorage& Vars)
{
	if (!Game::EvaluateCondition(Bhv.Condition, Session, &Vars)) return;

	if (Bhv.Params) Vars.Load(*Bhv.Params);

	if (Stack.pEffectData->Aggregated)
		RunBhvAggregated(Session, Stack, Bhv, Vars, Stack.AggregatedMagnitude);
	else
		for (const auto& Instance : Stack.Instances)
			if (IsInstanceActive(*Instance))
				RunBhvSeparate(Session, *Stack.pEffectData, *Instance, Bhv, Vars);
}
//---------------------------------------------------------------------

static void RemoveExpiredInstances(Game::CGameSession& Session, CStatusEffectStack& Stack, Game::CGameVarStorage& Vars)
{
	bool AggregatedMagnitudeChanged = false;
	for (auto ItInstance = Stack.Instances.begin(); ItInstance != Stack.Instances.end(); /**/)
	{
		auto& Instance = *(*ItInstance).get();
		if (Instance.Magnitude <= 0.f)
		{
			ItInstance = Stack.Instances.erase(ItInstance);
		}
		else if (Instance.ConditionSubs.empty() && !Game::EvaluateCondition(Instance.ValidityCondition, Session, &Vars))
		{
			const float PrevMagnitude = std::exchange(Instance.Magnitude, 0.f);
			if (!Instance.SuspendBehaviourCounter)
			{
				if (Stack.pEffectData->Aggregated)
					AggregatedMagnitudeChanged = true;
				else
					OnSeparateMagnitudeChanged(Session, *Stack.pEffectData, Instance, PrevMagnitude);
			}

			ItInstance = Stack.Instances.erase(ItInstance);
		}
		else
		{
			++ItInstance;
		}
	}

	if (AggregatedMagnitudeChanged)
		OnAggregatedMagnitudeChanged(Session, Stack, CalcAggregatedMagnitude(Stack));
}
//---------------------------------------------------------------------

static void TickInstanceRemainingTimes(Game::CGameSession& Session, CStatusEffectStack& Stack, float dt)
{
	bool AggregatedMagnitudeChanged = false;
	for (auto ItInstance = Stack.Instances.begin(); ItInstance != Stack.Instances.end(); /**/)
	{
		auto& Instance = *(*ItInstance).get();
		if (!Instance.SuspendLifetimeCounter)
		{
			if (Instance.RemainingTime <= dt)
			{
				const float PrevMagnitude = std::exchange(Instance.Magnitude, 0.f);
				if (!Instance.SuspendBehaviourCounter)
				{
					if (Stack.pEffectData->Aggregated)
						AggregatedMagnitudeChanged = true;
					else
						OnSeparateMagnitudeChanged(Session, *Stack.pEffectData, Instance, PrevMagnitude);
				}

				ItInstance = Stack.Instances.erase(ItInstance);
				continue;
			}

			Instance.RemainingTime -= dt;
		}

		Instance.Time += dt;
		++ItInstance;
	}

	if (AggregatedMagnitudeChanged)
		OnAggregatedMagnitudeChanged(Session, Stack, CalcAggregatedMagnitude(Stack));
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

static void RunOnTimeBehaviour(Game::CGameSession& Session, CStatusEffectStack& Stack, const CStatusEffectBehaviour& Bhv, Game::CGameVarStorage& Vars, float dt)
{
	// Can't process OnTime trigger without params
	n_assert(Bhv.Params);
	if (!Bhv.Params) return;

	if (!Game::EvaluateCondition(Bhv.Condition, Session, &Vars)) return;

	Vars.Load(*Bhv.Params);

	const float Delay = Bhv.Params->Get<float>(CStrID("Delay"), 0.f);
	const float Period = Bhv.Params->Get<float>(CStrID("Period"), 0.f);

	if (Stack.pEffectData->Aggregated)
	{
		// Tick processing requires special aggregation logic
		float Magnitude = 0.f;
		for (const auto& Instance : Stack.Instances)
		{
			if (!IsInstanceActive(*Instance)) continue;

			const auto ActivationCount = CalcTimeTriggerActivationCount(Instance->Time, dt, Delay, Period);
			if (!ActivationCount) continue;

			const float InstanceMagnitude = Instance->Magnitude * static_cast<float>(ActivationCount);

			if (Stack.pEffectData->MagnitudeAggregationPolicy == EStatusEffectNumMergePolicy::First)
			{
				Magnitude = InstanceMagnitude;
				break;
			}

			MergeValues(Magnitude, InstanceMagnitude, Stack.pEffectData->MagnitudeAggregationPolicy);
		}

		// TODO: apply magnitude limit? or not applicable because the same instance can tick multiple times?

		RunBhvAggregated(Session, Stack, Bhv, Vars, Magnitude);
	}
	else
	{
		for (const auto& Instance : Stack.Instances)
		{
			if (!IsInstanceActive(*Instance)) continue;

			const auto ActivationCount = CalcTimeTriggerActivationCount(Instance->Time, dt, Delay, Period);
			for (size_t i = 0; i < ActivationCount; ++i)
				RunBhvSeparate(Session, *Stack.pEffectData, *Instance, Bhv, Vars);
		}
	}
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
		Vars.Set(CStrID("StatusEffectOwner"), EntityID);

		//!!!FIXME: adding stacks in commands inside the loop can invalidate iteration!

		for (auto ItStack = StatusEffects.Stacks.begin(); ItStack != StatusEffects.Stacks.end(); /**/)
		{
			auto& [ID, Stack] = *ItStack;

			Vars.Set(CStrID("StatusEffectID"), Stack.pEffectData->ID);

			// Remove instances expired by conditions or magnitude before further processing.
			// Must check condition that has no subscriptions each frame. If a part of a condition
			// can't rely on signals, all subs are dropped.
			RemoveExpiredInstances(Session, Stack, Vars);

			if (!Stack.Instances.empty())
			{
				// Trigger OnTime behaviours on this stack
				auto ItBhvs = Stack.pEffectData->Behaviours.find(CStrID("OnTime"));
				if (ItBhvs != Stack.pEffectData->Behaviours.cend())
					for (const auto& Bhv : ItBhvs->second)
						RunOnTimeBehaviour(Session, Stack, Bhv, Vars, dt);

				// Advance instance time and remove instances expired by time
				TickInstanceRemainingTimes(Session, Stack, dt);
			}

			// Remove totally expired status effect stacks
			if (Stack.Instances.empty())
			{
				// OnRemoved is handled manually because it runs without instances and magnitude
				auto& Effect = *Stack.pEffectData;
				auto ItBhvs = Effect.Behaviours.find(CStrID("OnRemoved"));
				if (ItBhvs != Effect.Behaviours.cend())
				{
					Vars.Set(CStrID("StatusEffectID"), Effect.ID);

					for (const auto& Bhv : ItBhvs->second)
						if (Game::EvaluateCondition(Bhv.Condition, Session, &Vars))
							Game::ExecuteCommandList(Bhv.Commands, Session, &Vars);

					//???TODO: clear Vars from stack vars?
				}

				//!!!remove modifiers added by this stack! drop effects with this effect as a source?

				ItStack = StatusEffects.Stacks.erase(ItStack);

				// NB: a removed stack is not in the list already, and it is intentional
				ToggleSuspensionFromEffect(Session, StatusEffects, Effect, false);
			}
			else
			{
				++ItStack;
			}
		}
	});
}
//---------------------------------------------------------------------

}
