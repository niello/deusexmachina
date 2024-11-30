#pragma once
#include <Data/Metadata.h>
#include <Data/StringID.h>

// An objective of a player and its character. Can be started and then completed with different outcomes.

namespace DEM::RPG
{

struct CQuestData
{
	//CStrID      ID;
	CStrID      ParentID;
	std::string UIName;
	std::string UIDesc;
	// Outcomes: ID -> Name(?)+Desc+Rewards?
	//    if outcome not in list, is it impossible or simply not shown in UI and not giving a reward?
	// Flow asset or script for OnStart
	// Flow asset or script for outcome condition monitoring; OnOutcome logic can be stored in Lua object of the quest, for locality of data
	//    OnStarted, OnCompleted(Outcome, InOutRewards), OnReset; maybe also some arbitrary Lua functions called in flow nodes
	//???or store on started in the same flow? and when restoring quest, start player from remembered condition monitoring node
	//!!!flow can be inline, no resource manager needed!
	//???requirements (resources)?
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<RPG::CQuestData>() { return "DEM::RPG::CQuestData"; }
template<> constexpr auto RegisterMembers<RPG::CQuestData>()
{
	return std::make_tuple
	(
		//DEM_META_MEMBER_FIELD(RPG::CQuestData, ID),
		DEM_META_MEMBER_FIELD(RPG::CQuestData, ParentID),
		DEM_META_MEMBER_FIELD(RPG::CQuestData, UIName),
		DEM_META_MEMBER_FIELD(RPG::CQuestData, UIDesc)
	);
}
static_assert(CMetadata<RPG::CQuestData>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

}
