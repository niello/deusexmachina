#pragma once
#include <Game/ECS/ComponentStorage.h>
#include <Data/Metadata.h>

// Character AI state

namespace DEM::AI
{

struct CAIStateComponent
{
	CStrID        CurrInteraction;
	Game::HEntity CurrSmartObject;
	float         CurrInteractionTime = 0.f;
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::AI::CAIStateComponent>() { return "DEM::AI::CAIStateComponent"; }
template<> inline constexpr auto RegisterMembers<DEM::AI::CAIStateComponent>()
{
	return std::make_tuple
	(
		//Member(1, "Radius", &DEM::AI::CAIStateComponent::Radius, &AI::CAIStateComponent::Radius),
		//Member(2, "Height", &DEM::AI::CAIStateComponent::Height, &AI::CAIStateComponent::Height),
		//Member(3, "Settings", &DEM::AI::CAIStateComponent::SettingsID, &AI::CAIStateComponent::SettingsID)
	);
}

}
