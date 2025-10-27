#pragma once
#include <Scripting/Command.h>
#include <Scripting/Condition.h>

// Command list runs one or more commands with optional conditions and chance

namespace DEM::Game
{

struct CCommandListRecord
{
	CCommandData   Command;
	CConditionData Condition;             // Optional, empty condition type resolves to true
	float          Chance = 1.f;          // Chance >= 1.f = 100%, <=0.f is skipped, otherwise uses random chance check
	bool           BreakIfFailed = false;
};

using CCommandList = std::vector<CCommandListRecord>;

bool ExecuteCommandList(const CCommandList& List, CGameSession& Session, CGameVarStorage* pVars /*chance RNG*/);

inline bool ExecuteCommandList(const CCommandList& List, CGameSession& Session /*chance RNG*/)
{
	// A temporary context used only for communication inside the list during a single evaluation
	CGameVarStorage Vars;
	return ExecuteCommandList(List, Session, &Vars);
}
//---------------------------------------------------------------------

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<Game::CCommandListRecord>() { return "DEM::Game::CCommandListRecord"; }
template<> constexpr auto RegisterMembers<Game::CCommandListRecord>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(Game::CCommandListRecord, Command),
		DEM_META_MEMBER_FIELD(Game::CCommandListRecord, Condition),
		DEM_META_MEMBER_FIELD(Game::CCommandListRecord, Chance),
		DEM_META_MEMBER_FIELD(Game::CCommandListRecord, BreakIfFailed)
	);
}
static_assert(CMetadata<Game::CCommandListRecord>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

}
