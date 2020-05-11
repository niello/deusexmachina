#pragma once
#include <Physics/CharacterController.h>
#include <Physics/RigidBody.h> //!!!FIXME: required to generate _Body destruction, but why in header?!
#include <Data/Metadata.h>

// Character controller component adds a controllable physics object to the entity

namespace DEM::Game
{

enum class ECharacterState : U8
{
	Stand = 0, // Stands idle on feet
	Walk,      // Moves along the surface, facing the movement direction
	ShortStep, // Moves a short distance without changing face direction
	Jump,      // Above the ground, falls, controls itself
	Fall       // Above the ground, falls, control is lost
	// Lay (or switch to ragdoll?), Levitate
};

// FIXME: leave only data in component, split into controller, movement and facing requests etc!
struct CCharacterControllerComponent
{
	Physics::CCharacterController Controller;
	Physics::PRigidBody Body;

	float			Radius = 0.3f;
	float			Height = 1.75f;
	float			Hover = 0.2f;	//???is it MaxClimb itself?
	float			MaxStepDownHeight = 0.2f;	// Maximum height above the ground when character controls itself and doesn't fall
	float           MaxLinearSpeed = 3.f;
	float           MaxAngularSpeed = PI;
	float			MaxAcceleration = 0.f;
	float			MaxLandingImpulse;	//???speed? Maximum impulse the character can handle when landing. When exceeded, falling starts.
	//float			MaxJumpImpulse;		// Maximum jump impulse (mass- and direction-independent)
	float           BigTurnThreshold = PI / 3.f;        // Max angle (in rad) actor can turn without stopping linear movement
	float           SteeringSmoothness = 0.3f;
	float           ArriveBrakingCoeff = -0.5f / -10.f; // -1/2a = -0.5/a, where a is max brake acceleration, a < 0

	ECharacterState	State = ECharacterState::Stand;

	float GetRadius() const { return Controller.GetRadius(); }
	void  SetRadius(float Radius) { Controller.SetRadius(Radius); }
	float GetHeight() const { return Controller.GetHeight(); }
	void  SetHeight(float Height) { Controller.SetHeight(Height); }
	float GetHover() const { return Controller.GetHover(); }
	void  SetHover(float Hover) { Controller.SetHover(Hover); }

	bool IsOnTheGround() const { return State != ECharacterState::Jump && State != ECharacterState::Fall; } // Levitate too!
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
