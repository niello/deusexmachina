#pragma once
#include <Scripting/Flow/Condition.h>

// An objective of a player and its character. Can be started and then completed with different outcomes.

namespace DEM::RPG
{

struct CQuestOutcomeData
{
	std::string                            UIDesc;
	Flow::CConditionData                   Condition;
	std::vector<CStrID>                    StartQuests;
	std::vector<std::pair<CStrID, CStrID>> EndQuests; // Quest -> Outcome
	// Reward
	// Flow or Lua //???or single in CQuestData, outcome as arg?
};

struct CQuestData
{
	CStrID                                 ParentID;
	std::string                            UIName;
	std::string                            UIDesc;
	std::vector<CStrID>                    StartQuests;
	std::vector<std::pair<CStrID, CStrID>> EndQuests; // Quest -> Outcome
	std::map<CStrID, CQuestOutcomeData>    Outcomes;

	// Flow asset or script for OnStart
	// Flow asset or script for outcome condition monitoring; OnOutcome logic can be stored in Lua object of the quest, for locality of data
	//    OnStarted, OnCompleted(Outcome, InOutRewards), OnReset; maybe also some arbitrary Lua functions called in flow nodes
	//???or store on started in the same flow? and when restoring quest, start player from remembered condition monitoring node
	//!!!flow can be inline, no resource manager needed!

	// Requirements
	// OnStart Flow or Lua
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
		DEM_META_MEMBER_FIELD(RPG::CQuestData, Outcomes)
	);
}
static_assert(CMetadata<RPG::CQuestData>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

}
