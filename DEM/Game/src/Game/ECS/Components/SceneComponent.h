#pragma once
#include <Scene/SceneNode.h>
#include <Data/Metadata.h>

// Scene component contains a part of scene hierarchy, including a root node
// with transform and different possible scene node attributes.

namespace DEM::Game
{

struct CSceneComponent
{
	Scene::PSceneNode RootNode;

	CStrID            AssetID;
	std::string       RootPath;

	CSceneComponent() : RootNode(n_new(Scene::CSceneNode())) {}

	const auto& GetLocalTransform() const { return RootNode->GetLocalTransform(); }
	void        SetLocalTransform(const Math::CTransform& Value) { RootNode->SetLocalTransform(Value); }
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<quaternion>() { return "quaternion"; }
template<> inline constexpr auto RegisterMembers<quaternion>()
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

template<> inline constexpr auto RegisterClassName<Math::CTransform>() { return "Math::CTransform"; }
template<> inline constexpr auto RegisterMembers<Math::CTransform>()
{
	return std::make_tuple
	(
		Member(1, "S", &Math::CTransform::Scale, &Math::CTransform::Scale),
		Member(2, "R", &Math::CTransform::Rotation, &Math::CTransform::Rotation),
		Member(3, "T", &Math::CTransform::Translation, &Math::CTransform::Translation)
	);
}

template<> inline constexpr auto RegisterClassName<Game::CSceneComponent>() { return "DEM::Game::CSceneComponent"; }
template<> inline constexpr auto RegisterMembers<Game::CSceneComponent>()
{
	return std::make_tuple
	(
		Member(1, "AssetID", &Game::CSceneComponent::AssetID, &Game::CSceneComponent::AssetID),
		Member(2, "RootPath", &Game::CSceneComponent::RootPath, &Game::CSceneComponent::RootPath),
		Member<Game::CSceneComponent, Math::CTransform>(3, "Transform", &Game::CSceneComponent::GetLocalTransform, &Game::CSceneComponent::SetLocalTransform)
	);
}

}
