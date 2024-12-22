#include "HubAction.h"
#include <Scripting/Flow/FlowAsset.h>
#include <Core/Factory.h>

namespace DEM::Flow
{
FACTORY_CLASS_IMPL(DEM::Flow::CHubAction, 'HUBA', Flow::IFlowAction);

static const CStrID sidRandomNext("RandomNext");

const CFlowLink* CHubAction::ChooseNext(const CFlowActionData& Proto, Game::CGameSession& Session, const CBasicVarStorage& Vars, Math::CWELL512& RNG)
{
	const bool RandomNext = Proto.Params && Proto.Params->Get(sidRandomNext, false);
	return RandomNext ? GetRandomValidLink(Proto, Session, Vars, RNG) : GetFirstValidLink(Proto, Session, Vars);
}
//---------------------------------------------------------------------

void CHubAction::Update(Flow::CUpdateContext& Ctx)
{
	Goto(Ctx, ChooseNext(*_pPrototype, *Ctx.pSession, _pPlayer->GetVars(), _pPlayer->GetRNG()));
}
//---------------------------------------------------------------------

}
