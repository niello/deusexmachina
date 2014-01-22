#include "ActionSteerToPosition.h"

#include <AI/PropActorBrain.h>

namespace AI
{
__ImplementClass(AI::CActionSteerToPosition, 'ASTP', AI::CActionTraversePathEdge);

void CActionSteerToPosition::UpdatePathEdge(CActor* pActor, const CPathEdge* pEdge, const CPathEdge* pNextEdge)
{
	n_assert(pEdge);

	//!!!may want to arrive if:
	// big turn
	// next action requires arrive
	// real destination is within a slowdown radius //???rename DistToNavDest and use as final Arrive goal?

	pActor->GetMotorSystem().SetDest(pEdge->Dest);
	pActor->GetMotorSystem().SetNextDest(pNextEdge ? pNextEdge->Dest : pEdge->Dest);

	//???here or in motor system? big turn, next action doesn't use steering, don't check IsLast
	pActor->SteeringType = AISteer_Type_Seek; //(pEdge->IsLast || (pNextEdge && pNextEdge->IsLast)) ? AISteer_Type_Arrive : AISteer_Type_Seek;
}
//---------------------------------------------------------------------

DWORD CActionSteerToPosition::Update(CActor* pActor)
{
	switch (pActor->MvmtState)
	{
		case AIMvmt_Failed:
		{
			// Restart movement here!
			//!!!None can be set if we can't move with the selected movement type!
			//???or AIMvmt_Failed for this case?
			return Running;
		}
		case AIMvmt_DestSet:	return Running;
		case AIMvmt_Done:		return Success;
		case AIMvmt_Stuck:
		{
			// Process stuck, now just
			return Failure;

			// Remember that stuck can be released, so we may want to wait for some time here
			// getting stuck time from BlackBoard / MotorSystem
		}
		default: Core::Error("CActionSteerToPosition::Update(): Unexpected movement status '%d'", pActor->MvmtState);
	}
	return Failure;
}
//---------------------------------------------------------------------

void CActionSteerToPosition::Deactivate(CActor* pActor)
{
	if (pActor->MvmtState == AIMvmt_DestSet || pActor->MvmtState == AIMvmt_Stuck)
		pActor->GetMotorSystem().ResetMovement(false);

	if (pActor->FacingState == AIFacing_DirSet)
		pActor->GetMotorSystem().ResetRotation(false);
}
//---------------------------------------------------------------------

}