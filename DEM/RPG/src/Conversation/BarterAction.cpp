#include "BarterAction.h"
#include <Game/GameSession.h>
#include <Core/Factory.h>

namespace DEM::RPG
{
FACTORY_CLASS_IMPL(DEM::RPG::CBarterAction, 'BARA', Flow::IFlowAction);

static const CStrID sidActor("Actor");

void CBarterAction::OnStart(Game::CGameSession& Session)
{
	_Actor = ResolveEntityID(sidActor);
}
//---------------------------------------------------------------------

void CBarterAction::Update(Flow::CUpdateContext& Ctx)
{
	NOT_IMPLEMENTED_MSG("Barter");

	// TODO: here could also react on foreign currencies and unique items

	return Goto(Ctx, GetFirstValidLink(*Ctx.pSession, _pPlayer->GetVars()));
}
//---------------------------------------------------------------------

}
