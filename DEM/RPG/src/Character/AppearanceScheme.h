#pragma once
#include <Core/Object.h>
#include <Data/Metadata.h>
//#include <Data/StringID.h>
//#include <map>

// Stores settings for building a character appearance from parts

namespace DEM::RPG
{

class CAppearanceScheme : public ::Core::CObject
{
	RTTI_CLASS_DECL(CAppearanceScheme, ::Core::CObject);

public:

	// default visual parts
	// CVisualPart or map part to asset?
	// need additional attachments anyway
	//???need to define default parts with empty assets to explicitly show that they exist!
};

using PAppearanceScheme = Ptr<CAppearanceScheme>;

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::RPG::CAppearanceScheme>() { return "DEM::RPG::CAppearanceScheme"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CAppearanceScheme>()
{
	return std::make_tuple
	(
		//DEM_META_MEMBER_FIELD(RPG::CAppearanceScheme, 1, SlotTypes)
	);
}

}
