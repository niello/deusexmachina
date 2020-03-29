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
	CString           RootPath; // FIXME: use std::string, CData must use it too then

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
		Member(2, "RootPath", &Game::CSceneComponent::RootPath, &Game::CSceneComponent::RootPath),
		Member<Game::CSceneComponent, Math::CTransform>(3, "Transform", &Game::CSceneComponent::GetLocalTransform, &Game::CSceneComponent::SetLocalTransform)
	);
}

}
