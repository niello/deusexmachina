#include "ActionGotoSmartObj.h"

#include <AI/PropActorBrain.h>
#include <AI/PropSmartObject.h>
#include <Game/GameServer.h>
#include <Core/Factory.h>

namespace AI
{
__ImplementClass(AI::CActionGotoSmartObj, 'AGSO', AI::CActionGoto)

bool CActionGotoSmartObj::Activate(CActor* pActor)
{
	Game::CEntity* pEnt = GameSrv->GetEntityMgr()->GetEntity(TargetID);
	if (!pEnt) FAIL;
	Prop::CPropSmartObject* pSO = pEnt->GetProperty<Prop::CPropSmartObject>();
	if (!pSO || !pSO->IsActionAvailable(ActionID, pActor)) FAIL;

	RecoveryTime = 0.f;
	PolyCache.Clear();

	vector3 Dest;
	if (!pSO->GetRequiredActorPosition(ActionID, pActor, Dest, &PolyCache, true)) FAIL;
	pActor->GetNavSystem().SetDestPoint(Dest);
	State = State_Walk;

	OK;
}
//---------------------------------------------------------------------

UPTR CActionGotoSmartObj::Update(CActor* pActor)
{
	Game::CEntity* pEnt = GameSrv->GetEntityMgr()->GetEntity(TargetID);
	if (!pEnt) return Failure;
	Prop::CPropSmartObject* pSO = pEnt->GetProperty<Prop::CPropSmartObject>();
	if (!pSO || !pSO->IsActionAvailable(ActionID, pActor)) FAIL; //???IsActionAvailable() - some interval instead of every frame check?

	const vector3& TargetPos = pSO->GetEntity()->GetAttr<matrix44>(CStrID("Transform")).Translation();
	vector3 Dest;
	bool DestFound = pSO->GetRequiredActorPosition(ActionID, pActor, Dest, &PolyCache, PrevTargetPos != TargetPos);
	PrevTargetPos = TargetPos;

	switch (State)
	{
		case State_Walk:
		{
			if (!DestFound)
			{
				State = State_Chance;
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
					State = State_Chance;
					return Running;
				}
				case AINav_DestSet:		return Running;
				case AINav_Planning:
				case AINav_Following:	return AdvancePath(pActor);
				default: Sys::Error("CActionGotoSmartObj::Update(): Unexpected navigation status '%d'", pActor->NavState);
			}
		}
		case State_Face:
		{
			if (!DestFound)
			{
				State = State_Chance;
				return Running;
			}

			if (Dest != pActor->Position)
			{
				if (pActor->FacingState == AIFacing_DirSet)
					pActor->GetMotorSystem().ResetRotation(false);
				pActor->GetNavSystem().SetDestPoint(Dest);
				State = State_Walk;
				return Running;
			}

			//???call only if SO pos/orient changed?
			vector3 FaceDir;
			if (!pSO->GetRequiredActorFacing(ActionID, pActor, FaceDir)) return Success; // No facing required
			pActor->GetMotorSystem().SetFaceDirection(FaceDir);

			switch (pActor->FacingState)
			{
				case AIFacing_DirSet:	return Running;
				case AIFacing_Done:		return Success;
				case AIFacing_Failed:	return Failure;
				default: Sys::Error("CActionGotoSmartObj::Update() > Unexpected facing status '%d'", pActor->FacingState);
			}
		}
		case State_Chance:
		{
			if (DestFound)
			{
				RecoveryTime = 0.f;
				pActor->GetNavSystem().SetDestPoint(Dest);
				State = State_Walk;
				return Running;
			}

			RecoveryTime += (float)GameSrv->GetFrameTime();
			return RecoveryTime < pActor->NavDestRecoveryTime ? Running : Failure;
		}
		default: Sys::Error("CActionGotoSmartObj::Update() > Unknown state");
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