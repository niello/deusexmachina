#include "TaskUseSmartObj.h"

#include <AI/Prop/PropActorBrain.h>
#include <AI/Prop/PropSmartObject.h>
#include <AI/Behaviour/ActionSequence.h>
#include <AI/SmartObj/Actions/ActionGotoSmartObj.h>
#include <AI/SmartObj/Actions/ActionUseSmartObj.h>

namespace AI
{
__ImplementClassNoFactory(AI::CTaskUseSmartObj, AI::CTask);
__ImplementClass(AI::CTaskUseSmartObj);

bool CTaskUseSmartObj::IsAvailableTo(const CActor* pActor)
{
	//???could be use actions on self?!
	n_assert(pActor->GetEntity() != pSO->GetEntity());

	PSmartObjAction Action = pSO->GetAction(ActionID);

	//???also validate GotoSO action?
	return Action.IsValid() && Action->IsValid(pActor, pSO);
}
//---------------------------------------------------------------------

PAction CTaskUseSmartObj::BuildPlan()
{
	n_assert(pSO);

	PActionGotoSmartObj ActGoto = n_new(CActionGotoSmartObj);
	ActGoto->Init(pSO->GetEntity()->GetUID(), ActionID);

	PActionUseSmartObj ActUse = n_new(CActionUseSmartObj);
	ActUse->Init(pSO->GetEntity()->GetUID(), ActionID);

	PActionSequence Plan = n_new(CActionSequence);
	Plan->AddChild(ActGoto);
	Plan->AddChild(ActUse);

	return Plan.GetUnsafe();
}
//---------------------------------------------------------------------

} //namespace AI