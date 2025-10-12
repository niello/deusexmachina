#pragma once
#include <Scripting/Condition.h>

// An objective of a player and its character. Can be started and then completed with different outcomes.

namespace DEM::RPG
{

struct CQuestOutcomeData
{
	std::string                            UIDesc;
	Game::CConditionData                   Condition;
	std::vector<CStrID>                    StartQuests;
	std::vector<std::pair<CStrID, CStrID>> EndQuests; // Quest -> Outcome
	// Reward

	// For Lua binding compilation
	bool operator ==(const CQuestOutcomeData& Other) const noexcept { return UIDesc == Other.UIDesc; }
};

struct CQuestData
{
	CStrID                                            ParentID;
	std::string                                       UIName;
	std::string                                       UIDesc;
	std::vector<CStrID>                               StartQuests;
	std::vector<std::pair<CStrID, CStrID>>            EndQuests; // Quest -> Outcome
	std::vector<std::pair<CStrID, CQuestOutcomeData>> Outcomes; // Order is important
	CStrID                                            ScriptAssetID;
	// Requirements

	size_t FindOutcomeIndex(CStrID ID) const
	{
		for (size_t i = 0; i < Outcomes.size(); ++i)
			if (Outcomes[i].first == ID)
				return i;
		return std::numeric_limits<size_t>::max();
	}

	const CQuestOutcomeData* FindOutcome(CStrID ID) const
	{
		const auto OutcomeIndex = FindOutcomeIndex(ID);
		return (OutcomeIndex < Outcomes.size()) ? &Outcomes[OutcomeIndex].second : nullptr;
	}
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<RPG::CQuestOutcomeData>() { return "DEM::RPG::CQuestOutcomeData"; }
template<> constexpr auto RegisterMembers<RPG::CQuestOutcomeData>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CQuestOutcomeData, UIDesc),
		DEM_META_MEMBER_FIELD(RPG::CQuestOutcomeData, Condition),
		DEM_META_MEMBER_FIELD(RPG::CQuestOutcomeData, StartQuests),
		DEM_META_MEMBER_FIELD(RPG::CQuestOutcomeData, EndQuests)
	);
}
static_assert(CMetadata<RPG::CQuestOutcomeData>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

template<> constexpr auto RegisterClassName<RPG::CQuestData>() { return "DEM::RPG::CQuestData"; }
template<> constexpr auto RegisterMembers<RPG::CQuestData>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(RPG::CQuestData, ParentID),
		DEM_META_MEMBER_FIELD(RPG::CQuestData, UIName),
		DEM_META_MEMBER_FIELD(RPG::CQuestData, UIDesc),
		DEM_META_MEMBER_FIELD(RPG::CQuestData, StartQuests),
		DEM_META_MEMBER_FIELD(RPG::CQuestData, EndQuests),
		DEM_META_MEMBER_FIELD(RPG::CQuestData, Outcomes),
		DEM_META_MEMBER_FIELD(RPG::CQuestData, ScriptAssetID)
	);
}
static_assert(CMetadata<RPG::CQuestData>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

}
