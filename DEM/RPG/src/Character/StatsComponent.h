#pragma once
#include <Data/Metadata.h>
#include <StdDEM.h>

// A set of creature stats for Shantara2 role system

namespace DEM::Sh2
{

enum ECapability : U8 //!!!TODO: serialize U8 to CData!
{
	Move = 0x01,
	Interact = 0x02,
	Talk = 0x04
};

struct CStatsComponent
{
	//!!!TODO: serialize U8 to CData!
	//???need stats array indexed with enum? see CCharacterSheet.
	int Strength = 0;
	int Constitution = 0;
	int Dexterity = 0;
	int Perception = 0;
	int Erudition = 0;
	int Learnability = 0;
	int Charisma = 0;
	int Willpower = 0;
	U8  Capabilities = ECapability::Move | ECapability::Interact | ECapability::Talk; //???!!!serialize enum as String1 | String2 | ...!?
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<Sh2::CStatsComponent>() { return "DEM::Sh2::CStatsComponent"; }
template<> constexpr auto RegisterMembers<Sh2::CStatsComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, Strength),
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, Constitution),
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, Dexterity),
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, Perception),
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, Erudition),
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, Learnability),
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, Charisma),
		DEM_META_MEMBER_FIELD(Sh2::CStatsComponent, Willpower)
	);
}

}
