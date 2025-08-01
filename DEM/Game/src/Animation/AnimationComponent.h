#pragma once
#include <Animation/Skeleton.h>
#include <Animation/AnimationController.h>
#include <Data/Metadata.h>

// Animation component plays a single animation clip on the entity scene hierarchy

namespace DEM::Game
{

struct CAnimationComponent
{
	Anim::CSkeleton            Output;
	Anim::CAnimationController Controller;

	CStrID  ClipID;
	std::string RelTargetPath; // FIXME: use std::string, CData must use it too then
	float   Speed = 1.f;
	float   CurrTime = 0.f;
	bool    Looped = false;
	bool    Play = true;
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<Game::CAnimationComponent>() { return "DEM::Game::CAnimationComponent"; }
template<> constexpr auto RegisterMembers<Game::CAnimationComponent>()
{
	return std::make_tuple
	(
		Member(1, "ClipID", &Game::CAnimationComponent::ClipID, &Game::CAnimationComponent::ClipID),
		Member(2, "RelTargetPath", &Game::CAnimationComponent::RelTargetPath, &Game::CAnimationComponent::RelTargetPath),
		Member(3, "Speed", &Game::CAnimationComponent::Speed, &Game::CAnimationComponent::Speed),
		Member(4, "CurrTime", &Game::CAnimationComponent::CurrTime, &Game::CAnimationComponent::CurrTime),
		Member(5, "Looped", &Game::CAnimationComponent::Looped, &Game::CAnimationComponent::Looped),
		Member(6, "Play", &Game::CAnimationComponent::Play, &Game::CAnimationComponent::Play)
	);
}

}
