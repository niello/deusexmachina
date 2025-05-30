#pragma once
#include <Data/Metadata.h>

// A sensor that registers sound stimuli

namespace DEM::AI
{

struct CSoundSensorComponent
{
	float IntensityThreshold = 0.f;
	// node path & radius? or use collision shape of the host?
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<AI::CSoundSensorComponent>() { return "DEM::AI::CSoundSensorComponent"; }
template<> constexpr auto RegisterMembers<AI::CSoundSensorComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(AI::CSoundSensorComponent, IntensityThreshold)
	);
}

}
