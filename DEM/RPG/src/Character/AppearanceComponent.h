#pragma once
#include <Data/Metadata.h>
//#include <Character/AppearanceAsset.h>

// Character or creature appearance data for building a visual model

namespace DEM::RPG
{

struct CAppearanceComponent
{
	Data::PParams       Params; // FIXME: must not be shared!!!
	std::vector<CStrID> AppearanceAssets; // TODO: store asset reference here instead of ID?!

	// TODO: store currently instantiated visual parts (ID + parent bone -> node ptr), non-serializable
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
