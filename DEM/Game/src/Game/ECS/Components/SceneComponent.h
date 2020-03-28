#pragma once
#include <Data/StringID.h>
#include <Data/Metadata.h>

// Scene component contains a part of scene hierarchy, including a root node
// with transform and different possible scene node attributes.

namespace DEM::Game
{

struct CSceneComponent
{
	CStrID      AssetID;
	std::string ParentPath;
	// root path - empty = add asset to scene root, NOT instead of scene root
	// initial SRT //???in a node? instantiate asset inside it, not instead of it?

	// root node
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<Game::CSceneComponent>() { return "DEM::Game::CSceneComponent"; }
template<> inline constexpr auto RegisterMembers<Game::CSceneComponent>()
{
	return std::make_tuple
	(
		Member(1, "AssetID", &Game::CSceneComponent::AssetID, &Game::CSceneComponent::AssetID)
	);
}

}
