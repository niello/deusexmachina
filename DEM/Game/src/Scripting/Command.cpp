#include "Command.h"
#include <Scripting/LogicRegistry.h>
#include <Game/GameSession.h>

namespace DEM::Game
{

bool ExecuteCommand(const CCommandData& Command, CGameSession& Session, CGameVarStorage* pVars)
{
	// No-op
	if (!Command.Type) return true;

	const auto* pLogic = Session.FindFeature<CLogicRegistry>();
	if (!pLogic) return true;

	if (const auto& Cmd = pLogic->FindCommand(Command.Type))
		return Cmd(Session, Command.Params.Get(), pVars);

	::Sys::Error("Unsupported command type: {}"_format(Command.Type));
	return false;
}
//---------------------------------------------------------------------

float EvaluateCommandNumericValue(Game::CGameSession& Session, const Data::CParams* pParams, Game::CGameVarStorage* pVars, CStrID ID, float Default)
{
	if (auto* pData = pParams ? pParams->FindValue(ID) : nullptr)
	{
		if (const auto* pValue = pData->As<float>())
			return *pValue;

		if (const auto* pValue = pData->As<int>())
			return static_cast<float>(*pValue);

		if (const auto* pValue = pData->As<std::string>())
		{
			//???pass source and target sheets? what else? how to make flexible (easily add other data if needed later)?
			//???use environment instead of an immediate call of a temporary function?
			//???pass here or they must be passed from outside to every command, including this one? commands are also parametrized.
			//!!!can be even Vars.Get(Params.MyID)!
			//!!!also can call a function, even bound from C++! Formula can be like DEM.RPG.LinearStepwisePoison(Vars.Get('Magnitude')).
			sol::function Formula = Session.GetScriptState().script("return function(Params, Vars) return {} end"_format(*pValue));
			return Scripting::LuaCall<float>(Formula, pParams, pVars);
		}
	}

	return Default;
}
//---------------------------------------------------------------------

}
