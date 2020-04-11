#pragma once
#include <Physics/CharacterController.h>
#include <Physics/RigidBody.h> //!!!FIXME: required to generate _Body destruction, but why in header?!
#include <Data/Metadata.h>

// Character controller component adds a controllable physics object to the entity

namespace DEM::Game
{

// FIXME: leave only data in component, split into controller, movement and facing requests etc!
struct CCharacterControllerComponent
{
	Physics::CCharacterController Controller;

	float GetRadius() const { return Controller.GetRadius(); }
	void  SetRadius(float Radius) { Controller.SetRadius(Radius); }
	float GetHeight() const { return Controller.GetHeight(); }
	void  SetHeight(float Height) { Controller.SetHeight(Height); }
	float GetHover() const { return Controller.GetHover(); }
	void  SetHover(float Hover) { Controller.SetHover(Hover); }
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<Game::CCharacterControllerComponent>() { return "DEM::Game::CCharacterControllerComponent"; }
template<> inline constexpr auto RegisterMembers<Game::CCharacterControllerComponent>()
{
	return std::make_tuple
	(
		Member<Game::CCharacterControllerComponent, float>(1, "Radius", &Game::CCharacterControllerComponent::GetRadius, &Game::CCharacterControllerComponent::SetRadius),
		Member<Game::CCharacterControllerComponent, float>(2, "Height", &Game::CCharacterControllerComponent::GetHeight, &Game::CCharacterControllerComponent::SetHeight),
		Member<Game::CCharacterControllerComponent, float>(3, "Hover", &Game::CCharacterControllerComponent::GetHover, &Game::CCharacterControllerComponent::SetHover)
	);
}

}
