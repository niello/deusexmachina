#include "SetVarAction.h"
#include <Scripting/Flow/FlowAsset.h>
#include <Game/GameSession.h>
#include <Game/SessionVars.h>
#include <Core/Factory.h>

namespace DEM::Flow
{
FACTORY_CLASS_IMPL(DEM::Flow::CSetVarAction, 'SVRA', Flow::IFlowAction);

static const CStrID sidName("Name");
static const CStrID sidValue("Value");
static const CStrID sidStorage("Storage");
static const CStrID sidLocal("Local");
static const CStrID sidSession("Session");
static const CStrID sidPersistent("Persistent");

void CSetVarAction::Update(Flow::CUpdateContext& Ctx)
{
	if (const CStrID Name = _pPrototype->Params->Get<CStrID>(sidName, CStrID::Empty))
	{
		if (auto* pValueParam = _pPrototype->Params->Find(sidValue))
		{
			const CStrID Storage = _pPrototype->Params->Get<CStrID>(sidStorage, sidLocal);
			if (Storage == sidLocal)
			{
				_pPlayer->GetVars().TrySet(Name, pValueParam->GetRawValue());
			}
			else if (auto* pSessionVars = Ctx.pSession->FindFeature<Game::CSessionVars>())
			{
				if (Storage == sidSession)
					pSessionVars->Runtime.TrySet(Name, pValueParam->GetRawValue());
				else if (Storage == sidPersistent)
					pSessionVars->Persistent.TrySet(Name, pValueParam->GetRawValue());
				else
					return Throw(Ctx, "CSetVarAction: unknown storage type " + Storage.ToString(), false);
			}
			else
			{
				return Throw(Ctx, "CSetVarAction: CSessionVars is missing, can't set global (Session or Persistent) vars", false);
			}
		}
	}

	Goto(Ctx, GetFirstValidLink(*Ctx.pSession, _pPlayer->GetVars()));
}
//---------------------------------------------------------------------

}
