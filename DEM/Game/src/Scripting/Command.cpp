#include "Command.h"
#include <Scripting/LogicRegistry.h>
#include <Game/GameSession.h>

namespace DEM::Game
{

bool EvaluateCommand(const CCommandData& Command, CGameSession& Session, CGameVarStorage* pVars)
{
	// No-op
	if (!Command.Type) return true;

	const auto* pLogic = Session.FindFeature<CLogicRegistry>();
	if (!pLogic) return true;

	if (const auto& Cmd = pLogic->FindCommand(Command.Type))
	{
		Cmd(Session, Command.Params.Get(), pVars);
		return true;
	}

	::Sys::Error("Unsupported command type: {}"_format(Command.Type));
	return false;
}
//---------------------------------------------------------------------

}
