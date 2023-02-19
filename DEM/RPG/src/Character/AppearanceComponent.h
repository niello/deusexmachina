#pragma once
#include <Data/Metadata.h>
#include <Character/AppearanceScheme.h>

// Character or creature appearance data for building a visual model

namespace DEM::RPG
{

struct CAppearanceComponent
{
	CStrID            SchemeID; // FIXME: ID to resource directly!
	PAppearanceScheme Scheme;

	// additional parts/accessories
	// current state of the appearance - parts and attachments
};

}

namespace DEM::Meta
{

DEM_META_REGISTER_CLASS_NAME(DEM::RPG::CAppearanceComponent)
template<> inline constexpr auto RegisterMembers<DEM::RPG::CAppearanceComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CAppearanceComponent, 1, SchemeID)
	);
}

}
