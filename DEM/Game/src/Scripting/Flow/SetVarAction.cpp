#include "SetVarAction.h"
#include <Scripting/Flow/FlowAsset.h>
#include <Core/Factory.h>

namespace DEM::Flow
{
FACTORY_CLASS_IMPL(DEM::Flow::CSetVarAction, 'SVRA', Flow::IFlowAction);

static const CStrID sidName("Name");
static const CStrID sidValue("Value");

void CSetVarAction::Update(Flow::CUpdateContext& Ctx)
{
	if (const CStrID Name = _pPrototype->Params->Get<CStrID>(sidName, CStrID::Empty))
		if (auto* pValueParam = _pPrototype->Params->Find(sidValue))
			_pPlayer->GetVars().TrySet(Name, pValueParam->GetRawValue());

	Goto(Ctx, GetFirstValidLink(*Ctx.pSession, _pPlayer->GetVars()));
}
//---------------------------------------------------------------------

}
