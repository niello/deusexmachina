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
	const std::string_view Code = _pPrototype->Params->Get<std::string>(sidCode, EmptyString);
	if (!Code.empty())
	{
		sol::environment Env(Ctx.pSession->GetScriptState(), sol::create, Ctx.pSession->GetScriptState().globals());
		Env["Vars"] = &_pPlayer->GetVars();
		auto Result = Ctx.pSession->GetScriptState().script(Code, Env);
		if (!Result.valid())
			return Throw(Ctx, Result.get<sol::error>().what(), false);
	}

	Goto(Ctx, GetFirstValidLink(*Ctx.pSession, _pPlayer->GetVars()));
}
//---------------------------------------------------------------------

}
