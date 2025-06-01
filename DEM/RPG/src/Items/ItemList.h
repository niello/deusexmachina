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
struct CItemComponent;

struct CItemListRecord
{
	CStrID               ItemTemplateID;
	Resources::PResource SubList; // CItemList //!!!TODO: need an inline asset loading support! ID vs CParams!
	Flow::CConditionData Condition;
	U32                  RandomWeight = 1; // only for randomly evaluated lists
	U32                  MinCount = 1;
	U32                  MaxCount = 1;
	bool                 SingleEvaluation = false; // if true, a SubList will be evaluated once, and the result will be multiplied by Count
};

struct CItemLimits
{
	std::optional<U32>   Count;
	std::optional<U32>   Cost;
	std::optional<float> Weight;
	std::optional<float> Volume;
};

struct CItemStackData
{
	Game::HEntity         EntityID;
	const CItemComponent* pItem = nullptr;
	U32                   Count = 0;
};

class CItemList : public DEM::Core::CObject
{
	RTTI_CLASS_DECL(DEM::RPG::CItemList, DEM::Core::CObject);

protected:

	static bool EvaluateRecord(const CItemListRecord& Record, Game::CGameSession& Session, std::map<CStrID, CItemStackData>& Out, U32 Mul, CItemLimits& Limits);
	void EvaluateInternal(Game::CGameSession& Session, std::map<CStrID, CItemStackData>& Out, U32 Mul, CItemLimits& Limits) const;

public:

	std::vector<CItemListRecord> Records;

	std::optional<U32>           RecordLimit;
	CItemLimits                  ItemLimit;
	bool                         Random = true; // true - choose randomy, false - add one by one

	// TODO: decouple list and limits? and random flag too? they are generation params. Sub-lists will still need them. Support inline list assets?
	void Evaluate(Game::CGameSession& Session, std::map<CStrID, CItemStackData>& Out, const CItemLimits& Limits, U32 Mul = 1) const;
	void Evaluate(Game::CGameSession& Session, std::map<CStrID, CItemStackData>& Out, U32 Mul = 1) const { Evaluate(Session, Out, ItemLimit, Mul); }

	void OnPostLoad(Resources::CResourceManager& ResMgr);
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
		DEM_META_MEMBER_FIELD(RPG::CItemListRecord, SingleEvaluation)
	);
}
static_assert(CMetadata<RPG::CItemListRecord>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

template<> constexpr auto RegisterClassName<RPG::CItemLimits>() { return "DEM::RPG::CItemLimits"; }
template<> constexpr auto RegisterMembers<RPG::CItemLimits>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CItemLimits, Count),
		DEM_META_MEMBER_FIELD(RPG::CItemLimits, Cost),
		DEM_META_MEMBER_FIELD(RPG::CItemLimits, Weight),
		DEM_META_MEMBER_FIELD(RPG::CItemLimits, Volume)
	);
}
static_assert(CMetadata<RPG::CItemLimits>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

template<> constexpr auto RegisterClassName<RPG::CItemList>() { return "DEM::RPG::CItemList"; }
template<> constexpr auto RegisterMembers<RPG::CItemList>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CItemList, Records),
		DEM_META_MEMBER_FIELD(RPG::CItemList, RecordLimit),
		DEM_META_MEMBER_FIELD(RPG::CItemList, ItemLimit),
		DEM_META_MEMBER_FIELD(RPG::CItemList, Random)
	);
}
static_assert(CMetadata<RPG::CItemList>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

}
