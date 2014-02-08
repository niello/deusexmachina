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

	State = State_Walk;

	PolyCache.Clear();

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

	switch (State)
	{
		case State_Walk:
		{
			const vector3& TargetPos = pSO->GetEntity()->GetAttr<matrix44>(CStrID("Transform")).Translation();
			vector3 Dest;
			if (!pSO->GetRequiredActorPosition(ActionID, pActor, Dest, &PolyCache, PrevTargetPos != TargetPos))
			{
				// goto Chance
				return Running;
			}
			
			pActor->GetNavSystem().SetDestPoint(Dest);
			
			switch (pActor->NavState)
			{
				case AINav_Done:
				{
					vector3 FaceDir;
					if (!pSO->GetRequiredActorFacing(ActionID, pActor, FaceDir)) return Success; // No facing required
					pActor->GetMotorSystem().SetFaceDirection(FaceDir);
					State = State_Face;
					return Running;
				}
				case AINav_Failed:
				{
					// if no chance, return Failure;
					// goto Chance
					return Running;
				}
				case AINav_DestSet:		return Running;
				case AINav_Planning:
				case AINav_Following:	return AdvancePath(pActor);
				default: Core::Error("CActionGotoSmartObj::Update(): Unexpected navigation status '%d'", pActor->NavState);
			}
		}
		case State_Face:
		{
			// if pos [/ orient] of SO changed, update dest
			// if dest != curr actor pos, stop facing and goto Walk
			if (pActor->FacingState == AIFacing_DirSet)
				pActor->GetMotorSystem().ResetRotation(false);
			// if pos/ orient of SO changed, re-get direction of facing and set it to motor system
					//vector3 FaceDir;
					//if (!pSO->GetRequiredActorFacing(ActionID, pActor, FaceDir)) return Success; // No facing required
					//pActor->GetMotorSystem().SetFaceDirection(FaceDir);

			switch (pActor->FacingState)
			{
				case AIFacing_DirSet:	return Running;
				case AIFacing_Done:		return Success;
				case AIFacing_Failed:	return Failure;
				default: Core::Error("CActionGotoSmartObj::Update(): Unexpected facing status '%d'", pActor->FacingState);
			}
		}
		case State_Chance:
		{
			// stand still or try to go to the last pos
			//!!!chance threshold value must be stored in pActor and may be changed in runtime!
			//!!!reset failed dest timer when leave Chance!
			break;
		}
		default: Core::Error("CActionGotoSmartObj::Update > Unknown state");
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