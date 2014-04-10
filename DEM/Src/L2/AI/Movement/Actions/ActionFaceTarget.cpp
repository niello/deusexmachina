#include "ActionFaceTarget.h"

#include <AI/PropActorBrain.h>
#include <Game/EntityManager.h>

namespace AI
{
__ImplementClass(AI::CActionFaceTarget, 'AFTG', AI::CActionFace)

void CActionFaceTarget::Init(const Data::CParams& Desc)
{
	TargetID = GetStrID(Desc, CStrID("Target"));
}
//---------------------------------------------------------------------

bool CActionFaceTarget::Activate(CActor* pActor)
{
	if (!SetupDirFromTarget(pActor)) FAIL;

	//!!!Get IsDynamic as (BB->WantToFollow && IsTargetMovable)!
	IsDynamic = false;

	OK;
}
//---------------------------------------------------------------------

DWORD CActionFaceTarget::Update(CActor* pActor)
{
	if (IsDynamic && !SetupDirFromTarget(pActor)) return Failure;
	return CActionFace::Update(pActor);
}
//---------------------------------------------------------------------

bool CActionFaceTarget::SetupDirFromTarget(CActor* pActor)
{
	Game::CEntity* pEnt = EntityMgr->GetEntity(TargetID);
	if (!pEnt) FAIL;
	vector3 FaceDir = pEnt->GetAttr<matrix44>(CStrID("Transform")).Translation() - pActor->Position;
	FaceDir.norm();
	pActor->GetMotorSystem().SetFaceDirection(FaceDir);
	OK;
}
//---------------------------------------------------------------------

} //namespace AI