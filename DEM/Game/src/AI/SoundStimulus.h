#pragma once
#include <Game/ECS/Entity.h>

// Character AI state

namespace DEM::AI
{

struct CSoundStimulus
{
	Game::HEntity EntityID;
	vector3       Position;
	float         Intensity = 1.f;
	// types (tags)
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<DEM::AI::CSoundStimulus>() { return "DEM::AI::CSoundStimulus"; }
template<> constexpr auto RegisterMembers<DEM::AI::CSoundStimulus>()
{
	return std::make_tuple
	(
	);
}

}
