#pragma once
#include <Game/ECS/ComponentStorage.h>
#include <Game/Interaction/AbilityInstance.h>
#include <AI/Perception/Perception.h>
#include <AI/Blackboard.h>
#include <Data/Metadata.h>

// Character AI state

namespace DEM::AI
{

struct CAIStateComponent
{
	CBlackboard                  Blackboard;
	Game::PAbilityInstance       _AbilityInstance; // TODO: rename without _
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
		//Member(1, "Radius", &DEM::AI::CAIStateComponent::Radius, &AI::CAIStateComponent::Radius),
		//Member(2, "Height", &DEM::AI::CAIStateComponent::Height, &AI::CAIStateComponent::Height),
		//Member(3, "Settings", &DEM::AI::CAIStateComponent::SettingsID, &AI::CAIStateComponent::SettingsID)
	);
}

}
