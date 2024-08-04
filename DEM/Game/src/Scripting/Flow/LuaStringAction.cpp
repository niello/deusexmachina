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
	if (!Code.empty())
	{
		auto LuaFn = Ctx.pSession->GetScriptState().load("local Vars = ...; " + std::string(Code));
		if (!LuaFn.valid())
			return Throw(Ctx, LuaFn.get<sol::error>().what(), false);

		auto Result = LuaFn.get<sol::function>()(_pPlayer->GetVars());
		if (!Result.valid())
			return Throw(Ctx, Result.get<sol::error>().what(), false);
	}

	Goto(Ctx, GetFirstValidLink(*Ctx.pSession, _pPlayer->GetVars()));
}
//---------------------------------------------------------------------

}
