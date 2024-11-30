#pragma once
#include <Core/Object.h>
#include <Data/Metadata.h>
#include <Data/StringID.h>

// Quest is a graph of linked tasks representing a part of storyline, either main or optional
// This implementation manages only current tasks. Links are managed by tasks themselves.

namespace DEM::RPG
{

class CQuestData : public ::Core::CObject
{
	RTTI_CLASS_DECL(CEquipmentScheme, ::Core::CObject);

public:

	CStrID      ID;
	CStrID      ParentID;
	std::string UIName;
	std::string UIDesc;
};

using PQuestData = Ptr<CQuestData>;

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<DEM::RPG::CQuestData>() { return "DEM::RPG::CQuestData"; }
template<> inline constexpr auto RegisterMembers<DEM::RPG::CQuestData>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CQuestData, 1, ID),
		DEM_META_MEMBER_FIELD(RPG::CQuestData, 2, ParentID),
		DEM_META_MEMBER_FIELD(RPG::CQuestData, 3, UIName),
		DEM_META_MEMBER_FIELD(RPG::CQuestData, 4, UIDesc)
	);
}

}
