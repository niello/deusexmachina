#pragma once
#include <Core/Object.h>
#include <Game/ECS/Entity.h>
#include <Scripting/Condition.h>
#include <Scripting/CommandList.h>

// An effect that affects character stats or state

namespace DEM::RPG
{
constexpr float STATUS_EFFECT_INFINITE = std::numeric_limits<float>::infinity();

using PStatusEffectInstance = std::unique_ptr<struct CStatusEffectInstance>;
enum class EModifierType : U8;

enum class EStatusEffectNumMergePolicy : U8
{
	Sum,   // Sum of 2 values
	Max,   // Maximum of 2 values
	First, // Older value
	Last   // Newer value
};

enum class EStatusEffectSetMergePolicy : U8
{
	All,      // Set union (both subsets)
	Matching, // Set intersection (only equal elements)
	First,    // Older value
	Last,     // Newer value
	FullMatch // If values are not equal, merging fails
};

enum class EStatusEffectStackPolicy : U8
{
	Stack,        // Add a new instance
	Discard,      // Discard new instance
	Replace,      // Discard existing instances
	KeepLongest,  // Keep an instance with greater remaining duration
	KeepStrongest // Keep an instance with greater magnitude
};

struct CStatusEffectBehaviour
{
	Data::PParams        Params;
	Game::CConditionData Condition;
	Game::CCommandList   Commands;
};

class CStatusEffectData : public Core::CObject
{
	RTTI_CLASS_DECL(DEM::RPG::CStatusEffectData, DEM::Core::CObject);

public:

	CStrID           ID;
	std::set<CStrID> Tags;
	std::set<CStrID> SuspendBehaviourTags; // What effect must temporarily stop affecting the world
	std::set<CStrID> SuspendLifetimeTags;  // What effects must temporarily stop counting time to expire

	std::map<CStrID, std::vector<CStatusEffectBehaviour>> Behaviours;
	// - parametrized, like Health < 0.25 * TargetSheet.MaxHP
	// - if trigger is condition, it can subscribe an event with existing mechanism! or is trigger / condition a decorator too?
	// - trigger that has no event to react must be checked every frame? is it one of trigger types?
	//!!!need a list of trigger types (IDs)! Maybe use vector indexed by enum? Skip where command list is empty. How to deserialize?! Key = enum element name!
	//!!!try using command list in weapon/attack/ability/equipment first!

	// Stacking and aggregation parameters
	EStatusEffectStackPolicy    StackPolicy = EStatusEffectStackPolicy::Stack;
	EStatusEffectNumMergePolicy MagnitudeAggregationPolicy = EStatusEffectNumMergePolicy::Sum;
	float                       MaxAggregatedMagnitude = std::numeric_limits<float>::max(); //???apply to instances when no aggregation?
	bool                        Aggregated = true;

	// Merging parameters
	EStatusEffectNumMergePolicy MagnitudeMergePolicy = EStatusEffectNumMergePolicy::Sum;
	EStatusEffectNumMergePolicy DurationMergePolicy = EStatusEffectNumMergePolicy::Sum;
	EStatusEffectSetMergePolicy SourceMergePolicy = EStatusEffectSetMergePolicy::FullMatch;
	EStatusEffectSetMergePolicy TagMergePolicy = EStatusEffectSetMergePolicy::FullMatch;
	//float                       MaxInstanceDuration = STATUS_EFFECT_INFINITE;
	bool                        AllowMerge = true;

	// is hostile, is source known to target - or per command list or even per command? e.g. attack may not be a status effect but may use commands?
	// - each command can be hostile or not depending on the actual effect
	// - effect-applying command delegates hostility to the child effect, and it can apply it even at "on started/applied"
	//???use tags?

	// show in UI, icon, text with named param placeholders etc
	// UI name, desc, tooltip, possibly with hyperlinks

	//???precalculated CGameVarStorage context for commands? Can use same merge policy as tags, just rename it.

	void OnPostLoad()
	{
		Tags.insert(ID);
	}
};

struct CStatusEffectInstance
{
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
	struct CStatModifier
	{
		std::string          FormulaStr; //!!!could cache sol::function!
		const Data::CParams* pParams;
		EModifierType        Type;
		float                Value;
		U16                  Priority;
	};

	const CStatusEffectData*           pEffectData = nullptr; //???strong ptr? if resource, must have refcount anyway
	std::vector<PStatusEffectInstance> Instances; // Pointer to an instance must be stable, it can be captured in condition callbacks
	std::map<CStrID, CStatModifier>    ActiveMagnitudeStatModifiers;
	float                              AggregatedMagnitude = 0.f;
	const CStatusEffectInstance*       pAggregatedInstance = nullptr; // None when aggregated from multiple instances

	//???precalculated CGameVarStorage context for commands?
};

struct CStatusEffectsComponent
{
	struct CSuspendCounters
	{
		uint32_t Behaviour = 0;
		uint32_t Lifetime = 0;
	};

	std::map<CStrID, CStatusEffectStack> Stacks;
	std::map<CStrID, CSuspendCounters>   SuspendedTags;

	CStatusEffectsComponent() = default;
	CStatusEffectsComponent(CStatusEffectsComponent&&) noexcept = default;
	CStatusEffectsComponent& operator =(CStatusEffectsComponent&&) noexcept = default;
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
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectBehaviour, Commands)
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
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectData, SuspendBehaviourTags),
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectData, SuspendLifetimeTags),
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectData, Behaviours),
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectData, StackPolicy),
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectData, MagnitudeAggregationPolicy),
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectData, MaxAggregatedMagnitude),
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectData, Aggregated),
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectData, MagnitudeMergePolicy),
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectData, DurationMergePolicy),
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectData, SourceMergePolicy),
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectData, TagMergePolicy),
		DEM_META_MEMBER_FIELD(RPG::CStatusEffectData, AllowMerge)
	);
}
static_assert(CMetadata<RPG::CStatusEffectData>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

}
