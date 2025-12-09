#include "StatusEffectLogic.h"
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Character/StatsComponent.h>
#include <Scripting/LogicRegistry.h>

namespace DEM::RPG
{

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

	return Magnitude;
}
//---------------------------------------------------------------------

static void RunBhvAggregated(Game::CGameSession& Session, const CStatusEffectStack& Stack,
	const CStatusEffectBehaviour& Bhv, Game::CGameVarStorage& Vars, float Magnitude)
{
	if (Magnitude <= 0.f) return;

	// TODO: add merged source and tags info? All aggregations except Sum choose one active instance!

	// TODO: pass and modify duration the same way?
	const auto MagnitudeHandle = Vars.Set(CStrID("Magnitude"), Magnitude);
	Game::ExecuteCommandList(Bhv.Commands, Session, &Vars);
	const float NewMagnitude = std::max(Vars.Get<float>(MagnitudeHandle, 0.f), 0.f);

	if (NewMagnitude > Magnitude)
	{
		// TODO: has per instance upper limit? Must loop if overflow, but don't add new instances, discard extra magnitude!
		//???or must follow the overall aggregated limit? or both?
		Stack.Instances.back()->Magnitude += (NewMagnitude - Magnitude);
		// OnMagnitudeChanged
	}
	else if (NewMagnitude < Magnitude)
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

		// TODO: warn if RemainingDiff > 0

		// OnMagnitudeChanged
	}
}
//---------------------------------------------------------------------

static void RunBhvSeparate(Game::CGameSession& Session, CStatusEffectInstance& Instance, const CStatusEffectBehaviour& Bhv, Game::CGameVarStorage& Vars)
{
	const float Magnitude = Instance.Magnitude;

	// TODO: add source and tags info?

	// TODO: pass and modify duration the same way?
	const auto MagnitudeHandle = Vars.Set(CStrID("Magnitude"), Magnitude);
	Game::ExecuteCommandList(Bhv.Commands, Session, &Vars);
	const float NewMagnitude = std::max(Vars.Get<float>(MagnitudeHandle, 0.f), 0.f);

	if (NewMagnitude != Magnitude)
	{
		// TODO: has per instance upper limit?
		Instance.Magnitude = NewMagnitude;
		// OnMagnitudeChanged
	}
}
//---------------------------------------------------------------------

//???two methods? OnInstanceMagnitudeChanged, OnAggregatedMagnitudeChanged! First can call second!
static void OnMagnitudeChanged(Game::CGameSession& Session, CStatusEffectStack& Stack, CStatusEffectInstance& Instance, float PrevValue, float NewValue)
{
	if (Instance.SuspendBehaviourCounter || PrevValue == NewValue) return;

	if (Stack.pEffectData->AllowMerge)
	{
		//!!!if new aggregated magnitude passed as arg, should not recalculate! two separate methods?
		const float NewMagnitude = CalcAggregatedMagnitude(Stack);
		// TODO: apply magnitude limit
		if (Stack.AggregatedMagnitude == NewMagnitude) return;

		auto ItBhvs = Stack.pEffectData->Behaviours.find(CStrID("OnMagnitudeChanged"));
		if (ItBhvs == Stack.pEffectData->Behaviours.cend()) return;

		Game::CGameVarStorage Vars;
		//Vars.Set(CStrID("StatusEffectOwner"), EntityID);
		Vars.Set(CStrID("StatusEffectID"), Stack.pEffectData->ID);
		Vars.Set(CStrID("PrevMagnitude"), Stack.AggregatedMagnitude);
		Stack.AggregatedMagnitude = NewMagnitude;
		for (const auto& Bhv : ItBhvs->second)
			if (Game::EvaluateCondition(Bhv.Condition, Session, &Vars))
				RunBhvAggregated(Session, Stack, Bhv, Vars, NewMagnitude);
	}
	else
	{
		auto ItBhvs = Stack.pEffectData->Behaviours.find(CStrID("OnMagnitudeChanged"));
		if (ItBhvs == Stack.pEffectData->Behaviours.cend()) return;

		Game::CGameVarStorage Vars;
		//Vars.Set(CStrID("StatusEffectOwner"), EntityID);
		Vars.Set(CStrID("StatusEffectID"), Stack.pEffectData->ID);
		Vars.Set(CStrID("PrevMagnitude"), PrevValue);
		Instance.Magnitude = NewValue;
		for (const auto& Bhv : ItBhvs->second)
			if (Game::EvaluateCondition(Bhv.Condition, Session, &Vars))
				RunBhvSeparate(Session, Instance, Bhv, Vars);
	}

	//!!!update Stack modifiers based on magnitude (re-eval formulas)
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
			//???pass source and target sheets? what else? how to make flexible (easily add other data if needed later)? use environment?
			//???pass here or they must be passed from outside to every command, including this one? commands are also parametrized.
			//!!!can be even Vars.Get(Params.MyID)!
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
	if (!MagnitudeHandle) return false;

	float Magnitude = pVars->Get<float>(MagnitudeHandle, 0.f);

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

	const float PrevMagnitude = Dest.Magnitude;
	MergeValues(Dest.Magnitude, Src.Magnitude, Effect.MagnitudeMergePolicy);
	OnMagnitudeChanged(Dest, PrevMagnitude, Dest.Magnitude);

	return true;
}
//---------------------------------------------------------------------

// TODO: OnMagnitudeChanged
static void SetTagSuspension(CStatusEffectInstance& Instance, CStrID Tag, const CStatusEffectData& Effect, const CStatusEffectsComponent& Component)
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

// TODO: OnMagnitudeChanged
static void ToggleSuspensionFromEffect(CStatusEffectsComponent& Component, const CStatusEffectData& Effect, bool Enable)
{
	const int32_t Mod = Enable ? 1 : -1;

	for (const CStrID Tag : Effect.SuspendBehaviourTags)
	{
		Component.SuspendedTags[Tag].Behaviour += Mod;

		for (auto& [ID, Stack] : Component.Stacks)
		{
			// Always called before the stack is added or after it is removed
			n_assert_dbg(Effect.ID != ID);

			const bool HasTag = (Stack.pEffectData->Tags.find(Tag) != Stack.pEffectData->Tags.cend());
			for (auto& Instance : Stack.Instances)
				if (HasTag || (Instance->Tags.find(Tag) != Instance->Tags.cend()))
					Instance->SuspendBehaviourCounter += Mod;
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

	if (IsNewStack)
	{
		Stack.pEffectData = &Effect;

		// NB: a new stack is not in the list yet, and it is intentional
		ToggleSuspensionFromEffect(*pStatusEffectComponent, Effect, true);
	}
	else if (!Stack.Instances.empty())
	{
		// Try merging a new instance into an existing one if allowed
		if (Effect.AllowMerge)
			for (auto& ExistingInstance : Stack.Instances)
				if (MergeStatusEffectInstances(Effect, *ExistingInstance, *Instance))
					return true;


		// Choose between existing and the new instance if the stacking policy requires it
		switch (Effect.StackPolicy)
		{
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

	// Init suspension counters
	for (const CStrID Tag : Effect.Tags)
		SetTagSuspension(*Instance, Tag, Effect, *pStatusEffectComponent);
	for (const CStrID Tag : Instance->Tags)
		SetTagSuspension(*Instance, Tag, Effect, *pStatusEffectComponent);

	// Clear magnitude when expiration condition is met. This effectively invalidates an instance.
	if (const auto* pConditions = Session.FindFeature<Game::CLogicRegistry>())
	{
		if (auto* pCondition = pConditions->FindCondition(Instance->ValidityCondition.Type))
		{
			pCondition->SubscribeRelevantEvents(Instance->ConditionSubs, { Instance->ValidityCondition, Session, &Vars },
				[pInstance = Instance.get(), &Session](std::unique_ptr<Game::CGameVarStorage>& EventVars)
			{
				if (pInstance->Magnitude <= 0.f) return;

				if (!EventVars) EventVars = std::make_unique<Game::CGameVarStorage>();
				EventVars->Set(CStrID("SourceCreature"), pInstance->SourceCreatureID);
				EventVars->Set(CStrID("SourceItemStack"), pInstance->SourceItemStackID);

				if (!Game::EvaluateCondition(pInstance->ValidityCondition, Session, EventVars.get()))
				{
					// An instance expired by a condition
					pInstance->Magnitude = 0.f;
					OnMagnitudeChanged(*pInstance, pInstance->Magnitude, 0.f);
				}
			});
		}
	}

	// Add a new instance
	Stack.Instances.push_back(std::move(Instance));

	if (IsNewStack)
		TriggerStatusEffect(Session, Stack, CStrID("OnAdded"), Vars);

	OnMagnitudeChanged(Session, Stack, *Stack.Instances.back(), 0.f, Stack.Instances.back()->Magnitude);

	return true;
}
//---------------------------------------------------------------------

void RunStatusEffectBehaviour(Game::CGameSession& Session, const CStatusEffectStack& Stack, const CStatusEffectBehaviour& Bhv, Game::CGameVarStorage& Vars)
{
	if (!Game::EvaluateCondition(Bhv.Condition, Session, &Vars)) return;

	//???TODO: set Bhv.Params to Vars context?

	if (Stack.pEffectData->Aggregated)
		RunBhvAggregated(Session, Stack, Bhv, Vars, Stack.AggregatedMagnitude);
	else
		for (const auto& Instance : Stack.Instances)
			if (IsInstanceActive(*Instance))
				RunBhvSeparate(Session, *Instance, Bhv, Vars);
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

void RunStatusEffectOnTimeBehaviour(Game::CGameSession& Session, const CStatusEffectStack& Stack, const CStatusEffectBehaviour& Bhv, Game::CGameVarStorage& Vars, float dt)
{
	// Can't process OnTime trigger without params
	n_assert(Bhv.Params);
	if (!Bhv.Params) return;

	if (!Game::EvaluateCondition(Bhv.Condition, Session, &Vars)) return;

	//???TODO: set Bhv.Params to Vars context?

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
				RunBhvSeparate(Session, *Instance, Bhv, Vars);
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

		for (auto ItStack = StatusEffects.Stacks.begin(); ItStack != StatusEffects.Stacks.end(); /**/)
		{
			auto& [ID, Stack] = *ItStack;

			Vars.Set(CStrID("StatusEffectID"), Stack.pEffectData->ID);

			// Remove instances expired by conditions or magnitude before further processing.
			// Must check condition that has no subscriptions each frame. If a part of a condition
			// can't rely on signals, all subs are dropped.
			for (auto ItInstance = Stack.Instances.begin(); ItInstance != Stack.Instances.end(); /**/)
			{
				const auto& Instance = *(*ItInstance).get();
				if (Instance.Magnitude <= 0.f)
				{
					ItInstance = Stack.Instances.erase(ItInstance);
				}
				else if (Instance.ConditionSubs.empty() && !Game::EvaluateCondition(Instance.ValidityCondition, Session, &Vars))
				{
					OnMagnitudeChanged(Session, Stack, Instance, Instance.Magnitude, 0.f);
					ItInstance = Stack.Instances.erase(ItInstance);
				}
				else
				{
					++ItInstance;
				}
			}

			if (!Stack.Instances.empty())
			{
				// Trigger OnTime behaviours on this stack
				auto ItBhvs = Stack.pEffectData->Behaviours.find(CStrID("OnTime"));
				if (ItBhvs != Stack.pEffectData->Behaviours.cend())
					for (const auto& Bhv : ItBhvs->second)
						RunStatusEffectOnTimeBehaviour(Session, Stack, Bhv, Vars, dt)

				// Advance instance time and remove instances expired by time
				for (auto ItInstance = Stack.Instances.begin(); ItInstance != Stack.Instances.end(); /**/)
				{
					auto& Instance = *(*ItInstance).get();
					if (!Instance.SuspendLifetimeCounter)
					{
						if (Instance.RemainingTime <= dt)
						{
							OnMagnitudeChanged(Session, Stack, Instance, Instance.Magnitude, 0.f);
							ItInstance = Stack.Instances.erase(ItInstance);
							continue;
						}

						Instance.RemainingTime -= dt;
					}

					Instance.Time += dt;
					++ItInstance;
				}
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
				ToggleSuspensionFromEffect(StatusEffects, Effect, false);

				continue;
			}

			++ItStack;
		}
	});
}
//---------------------------------------------------------------------

// Find a current status effect instance for a command context
std::pair<CStatusEffectStack*, CStatusEffectInstance*> FindCurrentStatusEffectInstance(const Game::CGameSession& Session, const Game::CGameVarStorage& Vars)
{
	auto* pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return { nullptr, nullptr };

	const auto OwnerID = Vars.Get<Game::HEntity>(Vars.Find(CStrID("StatusEffectOwner")), {});
	auto* pStatusEffectComponent = pWorld->FindComponent<CStatusEffectsComponent>(OwnerID);
	if (!pStatusEffectComponent) return { nullptr, nullptr };

	const CStrID EffectID = Vars.Get<CStrID>(Vars.Find(CStrID("StatusEffectID")), {});
	auto ItStack = pStatusEffectComponent->Stacks.find(EffectID);
	if (ItStack == pStatusEffectComponent->Stacks.cend()) return { nullptr, nullptr };

	auto* pStack = &ItStack->second;
	const size_t InstanceIndex = static_cast<size_t>(Vars.Get<int>(Vars.Find(CStrID("StatusEffectInstanceIndex")), pStack->Instances.size()));
	return { pStack, (InstanceIndex < pStack->Instances.size()) ? pStack->Instances[InstanceIndex].get() : nullptr };
}
//---------------------------------------------------------------------

}
