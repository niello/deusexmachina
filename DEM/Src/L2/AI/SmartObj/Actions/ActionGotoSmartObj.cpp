#include "ActionGotoSmartObj.h"

#include <AI/PropActorBrain.h>
#include <AI/PropSmartObject.h>
#include <Game/EntityManager.h>

namespace AI
{
__ImplementClass(AI::CActionGotoSmartObj, 'AGSO', AI::CActionGoto)

bool CActionGotoSmartObj::Activate(CActor* pActor)
{
	Game::CEntity* pEnt = EntityMgr->GetEntity(TargetID);
	if (!pEnt) FAIL;
	CPropSmartObject* pSO = pEnt->GetProperty<CPropSmartObject>();
	if (!pSO || !pSO->IsActionAvailable(ActionID, pActor)) FAIL;

	PolyCache.Clear();
	IsFacing = false;

	vector3 Dest;
	if (!pSO->GetRequiredActorPosition(ActionID, pActor, Dest, &PolyCache, true)) FAIL;
	pActor->GetNavSystem().SetDestPoint(Dest);

	OK;
}
//---------------------------------------------------------------------

DWORD CActionGotoSmartObj::Update(CActor* pActor)
{
	Game::CEntity* pEnt = EntityMgr->GetEntity(TargetID);
	if (!pEnt) return Failure;
	CPropSmartObject* pSO = pEnt->GetProperty<CPropSmartObject>();
	if (!pSO || !pSO->IsActionAvailable(ActionID, pActor)) FAIL; //???IsActionAvailable() - some interval instead of every frame check?

//	if (pSO->IsMovable() && !UpdateNavDest(pActor, pSO)) return Failure;
/*
//???!!!need?!
if (pSO->IsActionAvailableFrom(ActionID, pActor->Position))
{
	pActor->GetNavSystem().Reset(true);
	//!!!reset failed dest timer! also reset in other places where needed!
	OK;
}
//!!!if failed dest timer is over, fail as target is lost!
//!!!threshold value must be stored in pActor and may be changed in runtime!
vector3 Dest;
if (!pSO->GetRequiredActorPosition(ActionID, pActor, Dest, &PolyCache, ...)) FAIL; //!!!keep a prev point some time! or fail?
pActor->GetNavSystem().SetDestPoint(Dest);
//!!!reset failed dest timer! also reset in other places where needed!
*/

	//!!!if navigation failed, replan path with updating cache!

	if (pActor->NavState == AINav_Done)
	{
		if (!IsFacing || pSO->IsMovable())
		{
			vector3 FaceDir;
			if (!pSO->GetRequiredActorFacing(ActionID, pActor, FaceDir)) return Success;
			pActor->GetMotorSystem().SetFaceDirection(FaceDir);
			IsFacing = true;
		}

		switch (pActor->FacingState)
		{
			case AIFacing_DirSet:	return Running;
			case AIFacing_Done:		return Success;
			case AIFacing_Failed:	return Failure;
			default: Core::Error("CActionGotoSmartObj::Update(): Unexpected facing status '%d'", pActor->FacingState);
		}
	}
	else
	{
		if (IsFacing)
		{
			if (pActor->FacingState == AIFacing_DirSet)
				pActor->GetMotorSystem().ResetRotation(false);
			IsFacing = false;
		}

		switch (pActor->NavState)
		{
			case AINav_Failed:		return Failure;
			case AINav_DestSet:		return Running;
			case AINav_Planning:
			case AINav_Following:	return AdvancePath(pActor);
			default: Core::Error("CActionGotoSmartObj::Update(): Unexpected navigation status '%d'", pActor->NavState);
		}
	}

	return Failure;
}
//---------------------------------------------------------------------

void CActionGotoSmartObj::Deactivate(CActor* pActor)
{
	PolyCache.Clear();
	if (pActor->FacingState == AIFacing_DirSet)
		pActor->GetMotorSystem().ResetRotation(false);
	CActionGoto::Deactivate(pActor);
}
//---------------------------------------------------------------------

}