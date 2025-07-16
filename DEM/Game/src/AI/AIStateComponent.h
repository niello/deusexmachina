#pragma once
#include <AI/Perception/Perception.h>
#include <AI/Blackboard.h>
#include <Data/Metadata.h>

// Character AI state

namespace DEM::AI
{

struct CAIStateComponent
{
	CBlackboard                  Blackboard;
	std::vector<CSensedStimulus> NewStimuli;
	std::vector<CSensedStimulus> Facts;
	size_t                       FactWithSourceCount = 0;
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<DEM::AI::CAIStateComponent>() { return "DEM::AI::CAIStateComponent"; }
template<> constexpr auto RegisterMembers<DEM::AI::CAIStateComponent>()
{
	return std::make_tuple
	(
	);
}

}
