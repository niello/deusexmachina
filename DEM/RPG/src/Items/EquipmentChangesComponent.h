#pragma once
#include <Data/Metadata.h>
#include <Game/ECS/Entity.h>

// Tracks changes in equipment, including equipping and unequipping items as long as
// changes in a set of components attached to the equipped item.
// This component is not persistent.

namespace DEM::RPG
{

struct CEquipmentChangesComponent
{
	struct CRecord
	{
		CStrID PrevSlot;
		CStrID NewSlot;
	};

	std::map<Game::HEntity, CRecord> Records;
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::RPG::CEquipmentChangesComponent>() { return "DEM::RPG::CEquipmentChangesComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CEquipmentChangesComponent>()
{
	return std::make_tuple(); // This component is not persistent.
}

}
