#pragma once
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <Scripting/Flow/Condition.h>

// Item list us used for random or conditional generation of items

namespace DEM::Game
{
	class CGameSession;
}

namespace DEM::RPG
{

struct CItemListRecord
{
	CStrID               ItemTemplateID;
	Resources::PResource SubList; // CItemList //!!!TODO: CItemList need a custom recursive loader!
	Flow::CConditionData Condition;
	U32                  RandomWeight = 1; // only for randomly evaluated lists

	// Can also use XdY+Z to implement non-linear distribution, but for a simple [min; max] it is harder to setup
	U32                  MinCount = 1;
	U32                  MaxCount = 1;
	bool                 SingleEval = false; // if true, a SubList will be evaluated once, and the result will be multiplied by Count
};

class CItemList : public ::Core::CObject
{
	RTTI_CLASS_DECL(DEM::RPG::CItemList, ::Core::CObject);

public:

	std::vector<CItemListRecord> Records;

	U32                          RecordLimit = 0;
	U32                          ItemLimit = 0;
	U32                          CostLimit = 0;
	U32                          WeightLimit = 0;
	U32                          VolumeLimit = 0;
	bool                         Random = true; // true - choose random, false - choose one by one

	void Evaluate(Game::CGameSession& Session, std::map<CStrID, U32>& Out);

	void OnPostLoad(Resources::CResourceManager& ResMgr)
	{
		// Recursively load sub-lists
		for (auto& Record : Records)
		{
			ResMgr.RegisterResource<CItemList>(Record.SubList);
			if (Record.SubList) Record.SubList->ValidateObject<CItemList>();
		}
	}
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<RPG::CItemListRecord>() { return "DEM::RPG::CItemListRecord"; }
template<> constexpr auto RegisterMembers<RPG::CItemListRecord>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CItemListRecord, ItemTemplateID),
		DEM_META_MEMBER_FIELD(RPG::CItemListRecord, SubList),
		DEM_META_MEMBER_FIELD(RPG::CItemListRecord, Condition),
		DEM_META_MEMBER_FIELD(RPG::CItemListRecord, RandomWeight),
		DEM_META_MEMBER_FIELD(RPG::CItemListRecord, MinCount),
		DEM_META_MEMBER_FIELD(RPG::CItemListRecord, MaxCount),
		DEM_META_MEMBER_FIELD(RPG::CItemListRecord, SingleEval)
	);
}
static_assert(CMetadata<RPG::CItemListRecord>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

template<> constexpr auto RegisterClassName<RPG::CItemList>() { return "DEM::RPG::CItemList"; }
template<> constexpr auto RegisterMembers<RPG::CItemList>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CItemList, Records),
		DEM_META_MEMBER_FIELD(RPG::CItemList, RecordLimit),
		DEM_META_MEMBER_FIELD(RPG::CItemList, ItemLimit),
		DEM_META_MEMBER_FIELD(RPG::CItemList, CostLimit),
		DEM_META_MEMBER_FIELD(RPG::CItemList, WeightLimit),
		DEM_META_MEMBER_FIELD(RPG::CItemList, VolumeLimit),
		DEM_META_MEMBER_FIELD(RPG::CItemList, Random)
	);
}
static_assert(CMetadata<RPG::CItemList>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

}
