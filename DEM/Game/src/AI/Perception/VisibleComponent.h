#pragma once
#include <Data/Metadata.h>

// This component makes an entity visible to CVisionSensor

namespace DEM::AI
{

struct CVisibleComponent
{
	float Visibility = 1.f;
	// TODO: move to RPG and store last detection check time and value here?
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<AI::CVisibleComponent>() { return "DEM::AI::CVisibleComponent"; }
template<> constexpr auto RegisterMembers<AI::CVisibleComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(AI::CVisibleComponent, Visibility)
	);
}

}
