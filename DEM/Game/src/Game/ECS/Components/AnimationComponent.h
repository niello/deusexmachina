#pragma once
#include <Animation/AnimationPlayer.h>
#include <Data/Metadata.h>

// Animation component plays a single animation clip on the entity scene hierarchy

namespace DEM::Game
{

struct CAnimationComponent
{
	DEM::Anim::PAnimationPlayer Player;

	CStrID  ClipID;
	CString RelTargetPath; // FIXME: use std::string, CData must use it too then
	float   Speed = 1.f;
	float   CurrTime = 0.f;
	bool    Looped = false;
	bool    Play = true;

	CAnimationComponent() = default;

	CAnimationComponent(const CAnimationComponent& Other)
		: ClipID(Other.ClipID), RelTargetPath(Other.RelTargetPath), Speed(Other.Speed)
		, CurrTime(Other.CurrTime), Looped(Other.Looped), Play(Other.Play)
	{}

	CAnimationComponent& operator =(const CAnimationComponent& Other)
	{
		ClipID = Other.ClipID;
		RelTargetPath = Other.RelTargetPath;
		Speed = Other.Speed;
		CurrTime = Other.CurrTime;
		Looped = Other.Looped;
		Play = Other.Play;
		return *this;
	}
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<Game::CAnimationComponent>() { return "DEM::Game::CAnimationComponent"; }
template<> inline constexpr auto RegisterMembers<Game::CAnimationComponent>()
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