#include "ActionTplUseSmartObj.h"

#include <AI/SmartObj/Actions/ActionUseSmartObj.h>
#include <AI/PropSmartObject.h>
#include <AI/Planning/WorldStateSource.h>
#include <AI/SmartObj/SmartAction.h>
#include <Game/Entity.h>
#include <Core/Factory.h>

namespace AI
{
FACTORY_CLASS_IMPL(AI::CActionTplUseSmartObj, 'ATUS', AI::CActionTpl);

void CActionTplUseSmartObj::Init(Data::PParams Params)
{
	CActionTpl::Init(Params);
	WSEffects.SetProp(WSP_UsingSmartObj, WSP_UsingSmartObj);
	WSEffects.SetProp(WSP_Action, WSP_Action);
}
//---------------------------------------------------------------------

bool CActionTplUseSmartObj::GetPreconditions(CActor* pActor, CWorldState& WS, const CWorldState& WSGoal) const
{
	//???check here is current position valid? or check somewhere else and DepartNode there?
	WS.SetProp(WSP_AtEntityPos, WSP_UsingSmartObj);

	// Request additional preconditions from SO action
	CStrID SOEntityID = WSGoal.GetProp(WSP_UsingSmartObj);
	CStrID ActionID = WSGoal.GetProp(WSP_Action);
	Prop::CPropSmartObject* pSO = nullptr;//GameSrv->GetEntityMgr()->GetProperty<Prop::CPropSmartObject>(SOEntityID);
	if (!pSO) OK;

	const Prop::CPropSmartObject::CAction* pSOAction = pSO->GetAction(ActionID);
	if (pSOAction && pSOAction->pTpl->Preconditions.IsValidPtr())
		pSOAction->pTpl->Preconditions->FillWorldState(pActor, pSO, WS);

	OK;
}
//---------------------------------------------------------------------

bool CActionTplUseSmartObj::ValidateContextPreconditions(CActor* pActor, const CWorldState& WSGoal)
{
	// Get pSO, Action
	// Check is action valid for planning (special PlannerContextValidator???)
	return WSGoal.IsPropSet(WSP_UsingSmartObj) && WSGoal.GetProp(WSP_UsingSmartObj) != CStrID::Empty;
}
//---------------------------------------------------------------------

PAction CActionTplUseSmartObj::CreateInstance(const CWorldState& Context) const
{
	PActionUseSmartObj Act = n_new(CActionUseSmartObj);
	Act->Init(Context.GetProp(WSP_UsingSmartObj), Context.GetProp(WSP_Action));
	return Act.Get();
}
//---------------------------------------------------------------------

}
