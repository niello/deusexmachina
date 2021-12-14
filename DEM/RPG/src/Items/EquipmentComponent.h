#pragma once
#include <Data/Metadata.h>
#include <StdDEM.h>

// Stores an equipment of a character, including quick slots, for a Shantara2 role system

namespace DEM::Sh2
{

enum EEquipmentSlot
{
	Body = 0,
	Shoulders,
	Head,
	Arms,
	Hands,
	Legs,
	Feet,
	Belt,
	Backpack,
	Neck,
	BraceletLeft,
	BraceletRight,
	Ring1Left,
	Ring1Right,
	Ring2Left,
	Ring2Right,

	COUNT
};

struct CEquipmentComponent
{
	Game::HEntity              Equipment[EEquipmentSlot::COUNT];
	std::vector<Game::HEntity> QuickSlots;
	U32                        SlotEnabledBits = 0;
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::Sh2::CEquipmentComponent>() { return "DEM::Sh2::CEquipmentComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::Sh2::CEquipmentComponent>()
{
	return std::make_tuple
	(
	);
}

}
