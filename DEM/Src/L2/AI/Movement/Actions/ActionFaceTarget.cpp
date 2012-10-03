#include "ActionFaceTarget.h"

#include <AI/Prop/PropActorBrain.h>
#include <Game/Mgr/EntityManager.h>

namespace Attr
{
	DeclareAttr(Transform);
}

namespace AI
{
ImplementRTTI(AI::CActionFaceTarget, AI::CActionFace)
ImplementFactory(AI::CActionFaceTarget);

bool CActionFaceTarget::Activate(CActor* pActor)
{
	if (!SetupDirFromTarget(pActor)) FAIL;

	//!!!Get IsDynamic as (BB->WantToFollow && IsTargetMovable)!
	IsDynamic = false;

	OK;
}
//---------------------------------------------------------------------

EExecStatus CActionFaceTarget::Update(CActor* pActor)
{
	if (IsDynamic && !SetupDirFromTarget(pActor)) return Failure;
	return CActionFace::Update(pActor);
}
//---------------------------------------------------------------------

bool CActionFaceTarget::SetupDirFromTarget(CActor* pActor)
{
	Game::CEntity* pEnt = EntityMgr->GetEntityByID(TargetID);
	if (!pEnt) FAIL;
	vector3 FaceDir = pEnt->Get<matrix44>(Attr::Transform).pos_component() - pActor->Position;
	FaceDir.norm();
	pActor->GetMotorSystem().SetFaceDirection(FaceDir);
	OK;
}
//---------------------------------------------------------------------

} //namespace AI