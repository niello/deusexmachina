#pragma once
#include <Data/Metadata.h>

// Character or creature appearance data for building a visual model

namespace Scene
{
	typedef Ptr<class CSceneNode> PSceneNode;
}

namespace DEM::RPG
{

struct CAppearanceComponent
{
	struct CLookPart
	{
		Scene::PSceneNode Node;
		Game::HEntity     SourceEntityID;
	};

	using CLookMap = std::map<std::pair<Resources::PResource, std::string>, CLookPart>;

	Data::PParams                     Params; // FIXME: must not be shared!!!
	std::vector<Resources::PResource> AppearanceAssets;
	CLookMap                          CurrentLook; // Not serialized
	std::set<Game::HEntity>           CurrentAttachments; // Not serialized
};

}

namespace DEM::Meta
{

DEM_META_REGISTER_CLASS_NAME(DEM::RPG::CAppearanceComponent)
template<> inline constexpr auto RegisterMembers<DEM::RPG::CAppearanceComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CAppearanceComponent, Params),
		DEM_META_MEMBER_FIELD(RPG::CAppearanceComponent, AppearanceAssets)
	);
}

}
