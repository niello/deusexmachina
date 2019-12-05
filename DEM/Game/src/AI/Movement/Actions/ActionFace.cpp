#include "ActionFace.h"

#include <AI/PropActorBrain.h>
#include <Core/Factory.h>

namespace AI
{
FACTORY_CLASS_IMPL(AI::CActionFace, 'AFAC', AI::CAction)

UPTR CActionFace::Update(CActor* pActor)
{
	switch (pActor->FacingState)
	{
		case AIFacing_DirSet:	return Running;
		case AIFacing_Done:		return Success;
		case AIFacing_Failed:	return Failure;
		default: Sys::Error("CActionFace::Update(): Unexpected facing status '%d'", pActor->FacingState);
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