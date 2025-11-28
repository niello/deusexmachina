#pragma once
#include <Core/Object.h>
#include <Data/StringID.h>
#include <Game/ECS/Entity.h>
#include <Scripting/Condition.h>
#include <Scripting/CommandList.h>

// An effect that affects character stats or state

namespace DEM::RPG
{
constexpr float STATUS_EFFECT_INFINITE = std::numeric_limits<float>::infinity();

using PStatusEffectInstance = std::unique_ptr<struct CStatusEffectInstance>;

enum class EStatusEffectMagnitudePolicy : U8
{
	Sum,
	Max,
	Oldest,
	Newest,
	Separate
};

struct CStatusEffectBehaviour
{
	Data::PParams                Params;
	Game::CConditionData         Condition;
	Game::CCommandList           Commands;
	EStatusEffectMagnitudePolicy MagnitudePolicy = EStatusEffectMagnitudePolicy::Sum;
};

class CStatusEffectData : public Core::CObject
{
	RTTI_CLASS_DECL(DEM::RPG::CStatusEffectData, DEM::Core::CObject);

public:

	CStrID           ID;
	std::set<CStrID> Tags;
	std::set<CStrID> BlockTags;            // What effects are discarded and blocked from being added
	std::set<CStrID> SuspendBehaviourTags; // What effect must temporarily stop affecting the world
	std::set<CStrID> SuspendLifetimeTags;  // What effects must temporarily stop counting time to expire

	std::map<CStrID, std::vector<CStatusEffectBehaviour>> Behaviours;
	// - parametrized, like Health < 0.25 * TargetSheet.MaxHP
	// - if trigger is condition, it can subscribe an event with existing mechanism! or is trigger / condition a decorator too?
	// - trigger that has no event to react must be checked every frame? is it one of trigger types?
	//!!!need a list of trigger types (IDs)! Maybe use vector indexed by enum? Skip where command list is empty. How to deserialize?! Key = enum element name!
	//!!!try using command list in weapon/attack/ability/equipment first!

	// stacking rules: max magnitude, max count, priority rule (prefer min, max, first, last when limiting by stacked instance count)
	//???magnitude - reduce in the instance or clamp stack sum after calculation?
	//???off-limit effects always suspend behaviour? or need configurable policy?

	// is hostile, is source known to target - or per command list or even per command? e.g. attack may not be a status effect but may use commands?
	// - each command can be hostile or not depending on the actual effect
	// - effect-applying command delegates hostility to the child effect, and it can apply it even at "on started/applied"

	// expiration conditions
	// show in UI, icon, text with named param placeholders etc
	// UI name, desc, tooltip, possibly with hyperlinks

	//???flag "only first instance"? for applying instances from different sources in a sequence. One expires, the next starts. Need?
};

struct CStatusEffectInstance
{
	//???merging rules here or in data?
	// Merging rules: is enabled, duration sum/max, magnitude sum/max. If disabled, stacking is performed. Merging happens to the instance of the same source ID.

	Game::HEntity                    SourceCreatureID;
	Game::HEntity                    SourceItemStackID;
	CStrID                           SourceAbilityID;
	CStrID                           SourceStatusEffectID;        // An effect must be applied by another "parent"/"source" effect

	std::set<CStrID>                 Tags;                        // Instances can have additional tags besides effect (stack) tags

	Game::CConditionData             ValidityCondition;           // As soon as it is false, an instance expires
	std::vector<Events::CConnection> ConditionSubs;

	float                            RemainingTime = STATUS_EFFECT_INFINITE;
	float                            Time = 0.f;
	float                            Magnitude = 0.f;             // When it drops to zero, an instance expires

	U8                               SuspendBehaviourCounter = 0; // While > 0, behaviours are not triggered and magnitude isn't contributed to the stack
	U8                               SuspendLifetimeCounter = 0;  // While > 0, internal timer doesn't tick and time-based expiration isn't checked

	//???position where an effect was applied, if applicable. E.g. for periodic blood leaking VFX. or custom var storage?
	//!!!in a custom var storage could store most of context info - sources etc!
};

struct CStatusEffectStack
{
	const CStatusEffectData*           pEffectData = nullptr; //???strong ptr? if resource, must have refcount anyway
	std::vector<PStatusEffectInstance> Instances; // Pointer to an instance must be stable, it can be captured in condition callbacks

	//???precalculated CGameVarStorage context for commands?

	// trigger and trigger condition event subscriptions

	// list of modified stats, if needed. Can skip this optiization for now. Or store map source->stats in a stats component!
	// Modifiers are applied per stack, with source = stack effect ID. Modifier is updated (or removed and re-added).
	// So each stat has only one modifiers from each affecting effect stack.

	// cached magnitude? or recalc each time? also may need a dirty flag
	// Total magnitude is recalculated each time and clamped to a min of all CStatusEffectData limits?
};

struct CStatusEffectsComponent
{
	std::map<CStrID, CStatusEffectStack> StatusEffectStacks;
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<RPG::CStatusEffectBehaviour>() { return "DEM::RPG::CStatusEffectBehaviour"; }
template<> constexpr auto RegisterMembers<RPG::CStatusEffectBehaviour>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectBehaviour, Params),
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectBehaviour, Condition),
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectBehaviour, Commands),
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectBehaviour, MagnitudePolicy)
	);
}
static_assert(CMetadata<RPG::CStatusEffectBehaviour>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

template<> constexpr auto RegisterClassName<RPG::CStatusEffectData>() { return "DEM::RPG::CStatusEffectData"; }
template<> constexpr auto RegisterMembers<RPG::CStatusEffectData>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectData, ID),
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectData, Tags),
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectData, BlockTags),
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectData, SuspendBehaviourTags),
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectData, SuspendLifetimeTags),
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectData, Behaviours)
	);
}
static_assert(CMetadata<RPG::CStatusEffectData>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

}
