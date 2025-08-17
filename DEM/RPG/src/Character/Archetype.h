#pragma once
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <Data/SerializeToParams.h>
#include <sol/sol.hpp>

// An archetype is a set of rules that defines specifics of a type of an RPG entity,
// typically a character or a creature. These specifics may include presence and limits
// of certain stats, formulas for secondary stats, list of body parts or hit zones etc.

namespace DEM::RPG
{

enum class ERoundingRule : U8
{
	None,
	Floor,
	Ceil,
	Nearest
};

struct CNumericStatDefinition
{
	sol::function Formula; //???!!!can / should cache here?
	std::string   FormulaStr;
	float         DefaultBaseValue = 0.f;
	float         MinBaseValue = std::numeric_limits<float>::lowest();
	float         MaxBaseValue = std::numeric_limits<float>::max();
	float         MinFinalValue = std::numeric_limits<float>::lowest();
	float         MaxFinalValue = std::numeric_limits<float>::max();
	ERoundingRule RoundingRule = ERoundingRule::None;
};

struct CBoolStatDefinition
{
	bool DefaultValue = false;
	// no formula, can't come up with an idea of its usage now
	// inverted name for scripts and facade value getter - for cases like IsNotCursed -> IsCursed
};

class CArchetype : public DEM::Core::CObject
{
	RTTI_CLASS_DECL(CArchetype, DEM::Core::CObject);

public:

	Resources::PResource                    BaseArchetype;

	std::unique_ptr<CNumericStatDefinition> Strength;

	std::unique_ptr<CBoolStatDefinition>    CanSpeak;

	std::set<CStrID>                        BodyParts; // TODO: name! Maybe HitZones?

	void OnPostLoad(Resources::CResourceManager& ResMgr)
	{
		ResMgr.RegisterResource<CArchetype>(BaseArchetype);
		if (BaseArchetype) BaseArchetype->ValidateObject<CArchetype>();

		// TODO:
		// maybe cache fallback pointers from base (need Ptr instead of unique_ptr then)
		// maybe cache Lua formulas in CNumericStatDefinition
		//???use reflection to iterate over CNumericStatDefinition fields?
		//???or use universal runtime map based on string IDs of stats from config?
	}
};

using PArchetype = Ptr<CArchetype>;

}

namespace DEM::Serialization
{

//!!!FIXME: need generic serialization for all enums!
template<>
struct ParamsFormat<DEM::RPG::ERoundingRule>
{
	static inline void Serialize(Data::CData& Output, DEM::RPG::ERoundingRule Value)
	{
		switch (Value)
		{
			case DEM::RPG::ERoundingRule::None: Output = std::string("None"); return;
			case DEM::RPG::ERoundingRule::Floor: Output = std::string("Floor"); return;
			case DEM::RPG::ERoundingRule::Ceil: Output = std::string("Ceil"); return;
			case DEM::RPG::ERoundingRule::Nearest: Output = std::string("Nearest"); return;
			default: Output = {}; return;
		}
	}

	static inline void Deserialize(const Data::CData& Input, DEM::RPG::ERoundingRule& Value)
	{
		if (!Input.IsA<std::string>()) return;

		const std::string_view InputStr = Input.GetValue<std::string>();
		if (InputStr == "None") Value = DEM::RPG::ERoundingRule::None;
		else if (InputStr == "Floor") Value = DEM::RPG::ERoundingRule::Floor;
		else if (InputStr == "Ceil") Value = DEM::RPG::ERoundingRule::Ceil;
		else if (InputStr == "Nearest") Value = DEM::RPG::ERoundingRule::Nearest;
	}
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<DEM::RPG::CNumericStatDefinition>() { return "DEM::RPG::CNumericStatDefinition"; }
template<> constexpr auto RegisterMembers<DEM::RPG::CNumericStatDefinition>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CNumericStatDefinition, FormulaStr),
		DEM_META_MEMBER_FIELD(RPG::CNumericStatDefinition, DefaultBaseValue),
		DEM_META_MEMBER_FIELD(RPG::CNumericStatDefinition, MinBaseValue),
		DEM_META_MEMBER_FIELD(RPG::CNumericStatDefinition, MaxBaseValue),
		DEM_META_MEMBER_FIELD(RPG::CNumericStatDefinition, MinFinalValue),
		DEM_META_MEMBER_FIELD(RPG::CNumericStatDefinition, MaxFinalValue),
		DEM_META_MEMBER_FIELD(RPG::CNumericStatDefinition, RoundingRule)
	);
}

template<> constexpr auto RegisterClassName<DEM::RPG::CBoolStatDefinition>() { return "DEM::RPG::CBoolStatDefinition"; }
template<> constexpr auto RegisterMembers<DEM::RPG::CBoolStatDefinition>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CBoolStatDefinition, DefaultValue)
	);
}

template<> constexpr auto RegisterClassName<DEM::RPG::CArchetype>() { return "DEM::RPG::CArchetype"; }
template<> constexpr auto RegisterMembers<DEM::RPG::CArchetype>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CArchetype, BaseArchetype),
		DEM_META_MEMBER_FIELD(RPG::CArchetype, Strength),
		DEM_META_MEMBER_FIELD(RPG::CArchetype, CanSpeak),
		DEM_META_MEMBER_FIELD(RPG::CArchetype, BodyParts)
	);
}

}
