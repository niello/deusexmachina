#pragma once
#include <Physics/RigidBody.h>
#include <Data/Metadata.h>

// Rigid body component runs a rigid body simulation on the entity,
// controlling its root transformation.

namespace DEM::Game
{

struct CRigidBodyComponent
{
	Physics::PRigidBody RigidBody;

	// TODO: shape params or asset ID
	// TODO: rel node path?
	// TODO: PhysicsMaterial
	// TODO: collision group and mask
	CStrID CollisionGroupID;
	CStrID CollisionMaskID;
	float Mass = 1.f;
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<Game::CRigidBodyComponent>() { return "DEM::Game::CRigidBodyComponent"; }
template<> inline constexpr auto RegisterMembers<Game::CRigidBodyComponent>()
{
	return std::make_tuple
	(
		Member(1, "CollisionGroupID", &Game::CRigidBodyComponent::CollisionGroupID, &Game::CRigidBodyComponent::CollisionGroupID),
		Member(2, "CollisionMaskID", &Game::CRigidBodyComponent::CollisionMaskID, &Game::CRigidBodyComponent::CollisionMaskID),
		Member(3, "Mass", &Game::CRigidBodyComponent::Mass, &Game::CRigidBodyComponent::Mass)
	);
}

}
