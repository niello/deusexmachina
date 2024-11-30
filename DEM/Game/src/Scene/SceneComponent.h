#pragma once
#include <Scene/SceneNode.h>
#include <Math/TransformSRT.h>
#include <Game/ECS/ComponentStorage.h>
#include <Math/SIMDMath.h>
#include <Data/Metadata.h>

// Scene component contains a part of scene hierarchy, including a root node
// with transform and different possible scene node attributes.

namespace DEM::Game
{

struct CSceneComponent
{
	// New scene nodes need to be attached to the scene.
	// Deleted entity scene nodes need to be detached from the scene.
	static constexpr bool Signals = true;

	Scene::PSceneNode RootNode;

	CStrID            AssetID;
	std::string       RootPath;

	CSceneComponent() : RootNode(n_new(Scene::CSceneNode())) {}

	// FIXME: can instead use RTM SIMD in metadata?!
	Math::CTransformSRT GetLocalTransform() const { return Math::FromSIMD(RootNode->GetLocalTransform()); }
	void                SetLocalTransform(const Math::CTransformSRT& Value) { RootNode->SetLocalTransform(Math::ToSIMD(Value)); }
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<quaternion>() { return "quaternion"; }
template<> constexpr auto RegisterMembers<quaternion>()
{
	// FIXME: quaternion diff will be too big! Can forbid diff saving for some types, save always all data in binary!
	return std::make_tuple
	(
		Member(1, "x", &quaternion::x, &quaternion::x),
		Member(2, "y", &quaternion::y, &quaternion::y),
		Member(3, "z", &quaternion::z, &quaternion::z),
		Member(4, "w", &quaternion::w, &quaternion::w)
	);
}

template<> constexpr auto RegisterClassName<Math::CTransformSRT>() { return "Math::CTransformSRT"; }
template<> constexpr auto RegisterMembers<Math::CTransformSRT>()
{
	return std::make_tuple
	(
		Member(1, "S", &Math::CTransformSRT::Scale, &Math::CTransformSRT::Scale),
		Member(2, "R", &Math::CTransformSRT::Rotation, &Math::CTransformSRT::Rotation),
		Member(3, "T", &Math::CTransformSRT::Translation, &Math::CTransformSRT::Translation)
	);
}

template<> constexpr auto RegisterClassName<Game::CSceneComponent>() { return "DEM::Game::CSceneComponent"; }
template<> constexpr auto RegisterMembers<Game::CSceneComponent>()
{
	return std::make_tuple
	(
		Member(1, "AssetID", &Game::CSceneComponent::AssetID, &Game::CSceneComponent::AssetID),
		Member(2, "RootPath", &Game::CSceneComponent::RootPath, &Game::CSceneComponent::RootPath),
		Member<Game::CSceneComponent, Math::CTransformSRT>(3, "Transform", &Game::CSceneComponent::GetLocalTransform, &Game::CSceneComponent::SetLocalTransform)
	);
}

}
