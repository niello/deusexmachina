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

bool EvaluateCommand(const CCommandData& Command, CGameSession& Session, CGameVarStorage* pVars);
//???GetCommandText?

//!!!TODO: scripted command here? or RegisterScriptedCommand wrapper in CLogicManager is enough?

//!!!command list here! vector<cmd + optional cond + optional chance + bool break on failure>

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
