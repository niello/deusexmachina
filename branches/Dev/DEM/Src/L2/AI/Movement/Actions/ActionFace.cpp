#include "ActionFace.h"

#include <AI/PropActorBrain.h>

namespace AI
{
__ImplementClass(AI::CActionFace, 'AFAC', AI::CAction)

DWORD CActionFace::Update(CActor* pActor)
{
	switch (pActor->FacingState)
	{
		case AIFacing_DirSet:	return Running;
		case AIFacing_Done:		return Success;
		case AIFacing_Failed:	return Failure;
		default: n_error("CActionFace::Update(): Unexpected facing status '%d'", pActor->FacingState);
	}

	return Failure;
}
//---------------------------------------------------------------------

void CActionFace::Deactivate(CActor* pActor)
{
	if (pActor->FacingState == AIFacing_DirSet)
		pActor->GetMotorSystem().ResetRotation(false);
}
//---------------------------------------------------------------------

}