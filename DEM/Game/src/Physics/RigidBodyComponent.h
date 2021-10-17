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

	// TODO: rel node path?
	// TODO: PhysicsMaterial - asset ID? Or enum?
	CStrID ShapeAssetID;
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
		Member(1, "ShapeAssetID", &Game::CRigidBodyComponent::ShapeAssetID, &Game::CRigidBodyComponent::ShapeAssetID),
		Member(2, "CollisionGroupID", &Game::CRigidBodyComponent::CollisionGroupID, &Game::CRigidBodyComponent::CollisionGroupID),
		Member(3, "CollisionMaskID", &Game::CRigidBodyComponent::CollisionMaskID, &Game::CRigidBodyComponent::CollisionMaskID),
		Member(4, "Mass", &Game::CRigidBodyComponent::Mass, &Game::CRigidBodyComponent::Mass)
	);
}

}
