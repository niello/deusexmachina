#pragma once
#include <Character/NumericStat.h>
#include <Character/BoolStat.h>
#include <Character/CharacterSheet.h>
#include <Resources/Resource.h>
#include <Data/Metadata.h>

// A set of creature stats for Shantara2 role system

namespace DEM::Sh2
{

struct CStatsComponent
{
	Resources::PResource Archetype;
	RPG::PCharacterSheet Sheet;

	RPG::CNumericStat Strength;
	RPG::CNumericStat Constitution;
	RPG::CNumericStat Dexterity;
	RPG::CNumericStat Perception;
	RPG::CNumericStat Erudition;
	RPG::CNumericStat Learnability;
	RPG::CNumericStat Charisma;
	RPG::CNumericStat Willpower;

	//!!!DBG TMP! here?
	RPG::CNumericStat BirthHP;
	RPG::CNumericStat MaxHP;

	RPG::CBoolStat    CanMove;
	RPG::CBoolStat    CanInteract;
	RPG::CBoolStat    CanSpeak;
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<Sh2::CStatsComponent>() { return "DEM::Sh2::CStatsComponent"; }
template<> constexpr auto RegisterMembers<Sh2::CStatsComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, Archetype),
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, Strength),
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, Constitution),
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, Dexterity),
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, Perception),
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, Erudition),
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, Learnability),
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, Charisma),
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, Willpower),
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, BirthHP),
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, MaxHP),
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, CanMove),
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, CanInteract),
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, CanSpeak)
	);
}
static_assert(CMetadata<Sh2::CStatsComponent>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

}
