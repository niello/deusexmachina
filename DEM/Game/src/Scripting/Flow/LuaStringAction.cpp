#include "LuaStringAction.h"
#include <Game/GameSession.h>
#include <Scripting/Flow/FlowAsset.h>
#include <Core/Factory.h>

namespace DEM::Flow
{
FACTORY_CLASS_IMPL(DEM::Flow::CLuaStringAction, 'LUSA', Flow::IFlowAction);

static const CStrID sidCode("Code");

void CLuaStringAction::Update(Flow::CUpdateContext& Ctx)
{
	const std::string_view Code = _pPrototype->Params->Get<CString>(sidCode, CString::Empty);
	if (Code.empty())
		return Goto(Ctx, GetFirstValidLink(*Ctx.pSession, _pPlayer->GetVars()));

	const auto Result = Ctx.pSession->GetScriptState().do_string(Code);
	if (Result.valid())
		return Goto(Ctx, GetFirstValidLink(*Ctx.pSession, _pPlayer->GetVars()));

	Throw(Ctx, Result.get<sol::error>().what(), false);
}
//---------------------------------------------------------------------

}
