#pragma once
#include <Game/GameVarStorage.h>
#include <Data/Params.h>
#include <Data/Metadata.h>

// Command object that can be described declaratively and executed in a provided context

namespace DEM::Game
{
class CGameSession;
using CCommand = std::function<void(CGameSession& Session, const Data::CParams* pParams, CGameVarStorage* pVars)>;

//???reuse between conditions and commands? CLogicData?
//!!!ResolveEntityID is also common!
struct CCommandData
{
	CStrID        Type;
	Data::PParams Params;
};

struct CCommandContext
{
	const CCommandData& Command;
	CGameSession&       Session;
	CGameVarStorage*    pVars = nullptr;
};

//???move to another header? or include condition here?
struct CCommandListRecord
{
	CCommandData   Command;
	CConditionData Condition;             // Optional, empty condition type resolves to true
	float          Chance = 1.f;          // Chance >= 1.f = 100%, <=0.f is skipped, otherwise uses random chance check
	bool           BreakIfFailed = false;
};

using CCommandList = std::vector<CCommandListRecord>;

bool ExecuteCommand(const CCommandData& Command, CGameSession& Session, CGameVarStorage* pVars);
bool ExecuteCommandList(const CCommandList& List, CGameSession& Session, CGameVarStorage* pVars /*chance RNG*/);
//???GetCommandText?

bool ExecuteCommandList(const CCommandList& List, CGameSession& Session /*chance RNG*/)
{
	// A temporary context used only for communication inside the list during a single evaluation
	CGameVarStorage Vars;
	return ExecuteCommandList(List, Session, &Vars);
}
//---------------------------------------------------------------------

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<Game::CCommandData>() { return "DEM::Game::CCommandData"; }
template<> constexpr auto RegisterMembers<Game::CCommandData>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(Game::CCommandData, Type),
		DEM_META_MEMBER_FIELD(Game::CCommandData, Params)
	);
}

}
