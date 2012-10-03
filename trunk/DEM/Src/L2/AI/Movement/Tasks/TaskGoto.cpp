#include "TaskGoto.h"

#include <AI/Prop/PropActorBrain.h>
#include <AI/Movement/Actions/ActionGoto.h>

namespace AI
{
ImplementRTTI(AI::CTaskGoto, AI::CTask);
ImplementFactory(AI::CTaskGoto);

// Can check movement capabilities here or just validate CActionGoto
bool CTaskGoto::IsAvailableTo(const CActor* pActor)
{
	n_assert(pActor);
	OK;
}
//---------------------------------------------------------------------

PAction CTaskGoto::BuildPlan()
{
	return n_new(CActionGoto);
}
//---------------------------------------------------------------------

bool CTaskGoto::OnPlanSet(CActor* pActor)
{
	n_assert(pActor);
	pActor->GetNavSystem().SetDestPoint(Point);
	pActor->MinReachDist = MinDistance;
	pActor->MaxReachDist = MaxDistance;
	pActor->MvmtType = MvmtType;
	OK;
}
//---------------------------------------------------------------------

} //namespace AI