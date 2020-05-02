#pragma once
#include <Data/Metadata.h>

// Navigation component allows an entity to plan path along navigation mesh

namespace DEM::AI
{

struct CNavigationComponent
{
	//???what is here, what is in request?

	//???navmesh query - here or in request?

	//???need at all, or navigation system must decompose Navigate action from DEM::AI::CActionQueue component?
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<AI::CNavigationComponent>() { return "DEM::Game::CNavigationComponent"; }
template<> inline constexpr auto RegisterMembers<AI::CNavigationComponent>()
{
	return std::make_tuple
	(
		//Member(1, "ClipID", &AI::CNavigationComponent::ClipID, &AI::CNavigationComponent::ClipID),
	);
}

}
