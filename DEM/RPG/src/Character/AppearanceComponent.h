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
	using CLookMap = std::map<std::pair<Resources::PResource, std::string>, Scene::PSceneNode>;

	Data::PParams                     Params; // FIXME: must not be shared!!!
	std::vector<Resources::PResource> AppearanceAssets;
	CLookMap                          CurrentLook; // Not serialized
};

}

namespace DEM::Meta
{

DEM_META_REGISTER_CLASS_NAME(DEM::RPG::CAppearanceComponent)
template<> inline constexpr auto RegisterMembers<DEM::RPG::CAppearanceComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CAppearanceComponent, 1, Params),
		DEM_META_MEMBER_FIELD(RPG::CAppearanceComponent, 2, AppearanceAssets)
	);
}

}
