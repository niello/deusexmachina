#pragma once
#include <Game/GameVarStorage.h>
#include <Data/Params.h>
#include <Data/Metadata.h>

// Command object that can be described declaratively and executed in a provided context

namespace DEM::Game
{
class CGameSession;
using CCommand = std::function<bool(CGameSession& Session, const Data::CParams* pParams, CGameVarStorage* pVars)>;

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

bool ExecuteCommand(const CCommandData& Command, CGameSession& Session, CGameVarStorage* pVars);
//???GetCommandText?

float EvaluateCommandNumericValue(Game::CGameSession& Session, const Data::CParams* pParams, Game::CGameVarStorage* pVars, CStrID ID, float Default);

template<typename T>
static T EvaluateCommandValue(const Data::CParams* pParams, CStrID ID, T Default)
{
	T Value = Default;
	Data::CData Data;
	if (pParams && pParams->TryGet(Data, ID))
		ParamsFormat::Deserialize(Data, Value);
	return Value;
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
static_assert(CMetadata<Game::CCommandData>::ValidateMembers()); // FIXME: how to trigger in RegisterMembers?

}
