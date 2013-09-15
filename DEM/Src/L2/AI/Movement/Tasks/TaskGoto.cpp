#include "TaskGoto.h"

#include <AI/PropActorBrain.h>
#include <AI/Movement/Actions/ActionGoto.h>

namespace AI
{
__ImplementClass(AI::CTaskGoto, 'TGOT', AI::CTask);

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

}