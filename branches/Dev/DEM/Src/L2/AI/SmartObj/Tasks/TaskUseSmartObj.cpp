#include "TaskUseSmartObj.h"

#include <AI/PropActorBrain.h>
#include <AI/PropSmartObject.h>
#include <AI/Behaviour/ActionSequence.h>
#include <AI/SmartObj/Actions/ActionGotoSmartObj.h>
#include <AI/SmartObj/Actions/ActionUseSmartObj.h>

namespace AI
{
__ImplementClass(AI::CTaskUseSmartObj, 'TUSO', AI::CTask);

bool CTaskUseSmartObj::IsAvailableTo(const CActor* pActor)
{
	//???could be using actions on self?! cast spell, use item.
	//???also validate GotoSO action?
	n_assert(pActor->GetEntity() != pSO->GetEntity());
	return pSO->IsActionAvailable(ActionID, pActor);
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

}