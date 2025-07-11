#pragma once
#include <Physics/RigidBody.h>
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
	// Lay (or switch to ragdoll?)
	// Levitate
};

struct CCharacterControllerComponent
{
	rtm::vector4f   LastPos; // For stuck state detection

	Physics::PRigidBody RigidBody;

	float           Mass = 80.f;
	float			Radius = 0.3f;
	float			Height = 1.75f;
	float			Hover = 0.2f;	//???is it MaxClimb itself?
	float			MaxStepDownHeight = 0.2f;	// Maximum height above the ground when character controls itself and doesn't fall
	float           MaxLinearSpeed = 3.f;
	float           MaxAngularSpeed = PI;
	float			MaxAcceleration = 0.f;
	float			MaxLandingVerticalSpeed = 3.f;	// Maximum Y-axis speed the character can handle when landing. When exceeded, falling starts.
	//float			MaxJumpImpulse;		// Maximum jump impulse (mass- and direction-independent)
	float           BigTurnThreshold = PI / 3.f;        // Max angle (in rad) actor can turn without stopping linear movement
	float           SteeringSmoothness = 0.3f;
	float           ArriveBrakingCoeff = -0.5f / -10.f; // -1/2a = -0.5/a, where a is max brake acceleration, a < 0

	// For stuck state detection
	float           SqExpectedLinearSpeed = 0.f;
	float           TimeStuck = 0.f;

	ECharacterState	State = ECharacterState::Stand;

	bool IsOnTheGround() const { return State != ECharacterState::Jump && State != ECharacterState::Fall; } // Check Levitate too!
	bool IsWalking() const { return State == ECharacterState::Walk || State == ECharacterState::ShortStep; }
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<Game::CCharacterControllerComponent>() { return "DEM::Game::CCharacterControllerComponent"; }
template<> constexpr auto RegisterMembers<Game::CCharacterControllerComponent>()
{
	return std::make_tuple
	(
		Member(1, "Radius", &Game::CCharacterControllerComponent::Radius, &Game::CCharacterControllerComponent::Radius),
		Member(2, "Height", &Game::CCharacterControllerComponent::Height, &Game::CCharacterControllerComponent::Height),
		Member(3, "Hover", &Game::CCharacterControllerComponent::Hover, &Game::CCharacterControllerComponent::Hover),
		Member(4, "MaxLinearSpeed", &Game::CCharacterControllerComponent::MaxLinearSpeed, &Game::CCharacterControllerComponent::MaxLinearSpeed),
		Member(5, "MaxAngularSpeed", &Game::CCharacterControllerComponent::MaxAngularSpeed, &Game::CCharacterControllerComponent::MaxAngularSpeed),
		Member(6, "MaxAcceleration", &Game::CCharacterControllerComponent::MaxAcceleration, &Game::CCharacterControllerComponent::MaxAcceleration)
	);
}

}
