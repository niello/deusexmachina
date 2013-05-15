#include "ActionFace.h"

#include <AI/Prop/PropActorBrain.h>

namespace AI
{
__ImplementClassNoFactory(AI::CActionFace, AI::CAction)
__ImplementClass(AI::CActionFace);

//bool CActionFace::Activate(CActor* pActor)
//{
//	// Derived classes can setup direction here before calling base method:
//	// pActor->GetMotorSystem().SetFaceDirection(...);
//	OK;
//}
////---------------------------------------------------------------------

EExecStatus CActionFace::Update(CActor* pActor)
{
	switch (pActor->FacingStatus)
	{
		case AIFacing_DirSet:	return Running;
		case AIFacing_Done:		return Success;
		case AIFacing_Failed:	return Failure;
		default: n_error("CActionFace::Update(): Unexpected facing status '%d'", pActor->FacingStatus);
	}

	return Failure;
}
//---------------------------------------------------------------------

void CActionFace::Deactivate(CActor* pActor)
{
	if (pActor->FacingStatus == AIFacing_DirSet)
		pActor->GetMotorSystem().ResetRotation();
}
//---------------------------------------------------------------------

//bool CActionGoto::IsValid(CActor* pActor) const
//{
//	return pActor->FaceDirSet;
//}
////---------------------------------------------------------------------

} //namespace AI