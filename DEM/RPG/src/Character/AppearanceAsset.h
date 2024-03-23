#pragma once
#include <Core/Object.h>
#include <Data/Metadata.h>
#include <Data/Params.h>

// Stores settings for building a character appearance from parts

namespace DEM::RPG
{

class CAppearanceAsset : public ::Core::CObject
{
	RTTI_CLASS_DECL(CAppearanceAsset, ::Core::CObject);

public:

	struct CVisualPartVariant
	{
		Resources::PResource Asset;
		Data::PParams        Conditions;
	};

	struct CVisualPart
	{
		std::vector<CStrID>             BodyParts;
		std::vector<CVisualPartVariant> Variants;
		std::optional<std::string>      RootBonePath;
	};

	std::vector<CVisualPart> Visuals;
};

using PAppearanceAsset = Ptr<CAppearanceAsset>;

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::RPG::CAppearanceAsset::CVisualPartVariant>() { return "DEM::RPG::CAppearanceAsset::CVisualPartVariant"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CAppearanceAsset::CVisualPartVariant>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CAppearanceAsset::CVisualPartVariant, 1, Asset),
		DEM_META_MEMBER_FIELD(RPG::CAppearanceAsset::CVisualPartVariant, 2, Conditions)
	);
}

template<> inline constexpr auto RegisterClassName<DEM::RPG::CAppearanceAsset::CVisualPart>() { return "DEM::RPG::CAppearanceAsset::CVisualPart"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CAppearanceAsset::CVisualPart>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CAppearanceAsset::CVisualPart, 1, BodyParts),
		DEM_META_MEMBER_FIELD(RPG::CAppearanceAsset::CVisualPart, 2, Variants),
		DEM_META_MEMBER_FIELD(RPG::CAppearanceAsset::CVisualPart, 3, RootBonePath)
	);
}

template<> inline constexpr auto RegisterClassName<DEM::RPG::CAppearanceAsset>() { return "DEM::RPG::CAppearanceAsset"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CAppearanceAsset>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CAppearanceAsset, 1, Visuals)
	);
}

}
