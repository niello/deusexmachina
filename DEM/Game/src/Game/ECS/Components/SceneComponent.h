#pragma once
#include <Scene/SceneNode.h>
#include <Data/Metadata.h>

// Scene component contains a part of scene hierarchy, including a root node
// with transform and different possible scene node attributes.

namespace DEM::Game
{

struct CSceneComponent
{
	CStrID            AssetID;
	std::string       ParentPath;
	// root path - empty = add asset to scene root, NOT instead of scene root
	// initial SRT //???in a node? instantiate asset inside it, not instead of it?

	Scene::PSceneNode RootNode;

	CSceneComponent() : RootNode(n_new(Scene::CSceneNode())) {}

	const auto& GetLocalTransform() const { return RootNode->GetLocalTransform(); }
	void        SetLocalTransform(const Math::CTransform& Value) { RootNode->SetLocalTransform(Value); }
};

}

namespace DEM::Meta
{

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
		Member<Game::CSceneComponent, Math::CTransform>(2, "Transform", &Game::CSceneComponent::GetLocalTransform, &Game::CSceneComponent::SetLocalTransform)
	);
}

}
