#pragma once
#include <Game/ECS/Entity.h>

// Character AI state

namespace DEM::AI
{

struct CActiveStimulus
{
	Game::HEntity EntityID;
	vector3       Position;
	float         Intensity = 1.f;
	// types (tags)
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<DEM::AI::CActiveStimulus>() { return "DEM::AI::CActiveStimulus"; }
template<> constexpr auto RegisterMembers<DEM::AI::CActiveStimulus>()
{
	return std::make_tuple
	(
	);
}

}
