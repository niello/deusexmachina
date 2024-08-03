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
	{
		if (auto* pValueParam = _pPrototype->Params->Find(sidValue))
		{
			if (auto* pValue = pValueParam->GetRawValue().As<bool>())
				_pPlayer->GetVars().Set(Name, *pValue);
			else if (auto* pValue = pValueParam->GetRawValue().As<int>())
				_pPlayer->GetVars().Set(Name, *pValue);
			else if (auto* pValue = pValueParam->GetRawValue().As<float>())
				_pPlayer->GetVars().Set(Name, *pValue);
			else if (auto* pValue = pValueParam->GetRawValue().As<CStrID>())
				_pPlayer->GetVars().Set(Name, *pValue);
			else if (auto* pValue = pValueParam->GetRawValue().As<CString>())
				_pPlayer->GetVars().Set(Name, pValue->CStr());
		}
	}

	Goto(Ctx, GetFirstValidLink(*Ctx.pSession, _pPlayer->GetVars()));
}
//---------------------------------------------------------------------

}
