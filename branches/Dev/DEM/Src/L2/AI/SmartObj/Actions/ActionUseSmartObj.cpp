#include "ActionUseSmartObj.h"

#include <AI/PropActorBrain.h>
#include <AI/PropSmartObject.h>
#include <AI/Movement/Actions/ActionFace.h>
#include <Game/EntityManager.h>
#include <Game/GameServer.h>
#include <Events/EventServer.h>

namespace AI
{
__ImplementClass(AI::CActionUseSmartObj, 'AUSO', AI::CAction)

using namespace Prop;
using namespace Data;

void CActionUseSmartObj::StartSOAction(CActor* pActor, Prop::CPropSmartObject* pSO, CSmartObjAction* pSOAction)
{
	PParams P = n_new(CParams(3));
	P->Set(CStrID("Actor"), pActor->GetEntity()->GetUID());
	P->Set(CStrID("SO"), TargetID);
	P->Set(CStrID("Action"), ActionID);
	if (pSOAction->OnStartCmd.IsValid()) pSO->GetEntity()->FireEvent(pSOAction->OnStartCmd, P);
	EventSrv->FireEvent(CStrID("OnSOActionStart"), P);

	//SOANIM
	//!!!play anim for actor and for SO, if it has one for this action!

	if (pSOAction->FreeUserSlots > 0) pSOAction->FreeUserSlots--;
}
//---------------------------------------------------------------------

bool CActionUseSmartObj::Activate(CActor* pActor)
{
	Game::CEntity* pSOEntity = EntityMgr->GetEntity(TargetID);
	if (!pSOEntity || pSOEntity->GetLevel() != pActor->GetEntity()->GetLevel()) FAIL;
	CPropSmartObject* pSO = pSOEntity->GetProperty<CPropSmartObject>();
	if (!pSO) FAIL;
	CSmartObjAction* pSOAction = pSO->GetAction(ActionID);
	if (!pSOAction || !pSOAction->IsValid(pActor, pSO)) FAIL;

	WasDone = false;

	if (pSOAction->FaceObject())
	{
		vector3 FaceDir = pSOEntity->GetAttr<matrix44>(CStrID("Transform")).Translation() - pActor->Position;
		FaceDir.norm();
		pActor->GetMotorSystem().SetFaceDirection(FaceDir);
		SubActFace = n_new(CActionFace);
		return SubActFace->Activate(pActor);
	}
	else StartSOAction(pActor, pSO, pSOAction);

	OK;
}
//---------------------------------------------------------------------

EExecStatus CActionUseSmartObj::Update(CActor* pActor)
{
	Game::CEntity* pSOEntity = EntityMgr->GetEntity(TargetID);
	if (!pSOEntity || pSOEntity->GetLevel() != pActor->GetEntity()->GetLevel()) return Failure;
	CPropSmartObject* pSO = pSOEntity->GetProperty<CPropSmartObject>();
	if (!pSO) return Failure;
	CSmartObjAction* pSOAction = pSO->GetAction(ActionID);
	if (!pSOAction) return Failure;

	// Finish SO facing if started
	if (SubActFace.IsValid())
	{
		EExecStatus Status = SubActFace->Update(pActor);
		switch (Status)
		{
			case Running: return Running;
			case Success:
				SubActFace->Deactivate(pActor);
				SubActFace = NULL;
				StartSOAction(pActor, pSO, pSOAction);
				break;
			case Failure:
			case Error:
				SubActFace->Deactivate(pActor);
				SubActFace = NULL;
				return Status;
		}
	}

	// Update anim here

	if (WasDone) return Running;

	pSOAction->Progress += (float)GameSrv->GetFrameTime();

	float Duration = pSOAction->GetTpl().Duration;
	if (Duration >= 0.f && pSOAction->Progress >= Duration)
	{
		if (pSOAction->Resource > 0) pSOAction->Resource--;

		WasDone = true;
		pSOAction->Progress = 0.f;

		// SmartObj can be destroyed on action done, so cache this value
		bool EndOnDone = pSOAction->EndOnDone();

		PParams P = n_new(CParams(3));
		P->Set(CStrID("Actor"), pActor->GetEntity()->GetUID());
		P->Set(CStrID("SO"), TargetID);
		P->Set(CStrID("Action"), ActionID);
		if (pSOAction->OnDoneCmd.IsValid()) pSOEntity->FireEvent(pSOAction->OnDoneCmd, P);
		EventSrv->FireEvent(CStrID("OnSOActionDone"), P);

		if (EndOnDone) return Success;
	}
	//else if duration is applicable
		//???flag Action->FireProgressChangeEvent?
		//???need to trigger some script function or fire OnSOActionProgressChanged event if progress changes?

	return Running;
}
//---------------------------------------------------------------------

void CActionUseSmartObj::Deactivate(CActor* pActor)
{
	if (SubActFace.IsValid())
	{
		SubActFace->Deactivate(pActor);
		SubActFace = NULL;
		return;
	}

	Game::CEntity* pSOEntity = EntityMgr->GetEntity(TargetID);
	if (!pSOEntity || pSOEntity->GetLevel() != pActor->GetEntity()->GetLevel()) return;
	CPropSmartObject* pSO = pSOEntity->GetProperty<CPropSmartObject>();
	if (!pSO) return;
	CSmartObjAction* pSOAction = pSO->GetAction(ActionID);
	if (!pSOAction) return;

	if (pSOAction->FreeUserSlots >= 0) pSOAction->FreeUserSlots++;
	
	PParams P = n_new(CParams(3));
	P->Set(CStrID("Actor"), pActor->GetEntity()->GetUID());
	P->Set(CStrID("SO"), TargetID);
	P->Set(CStrID("Action"), ActionID);

	if (WasDone)
	{
		if (pSOAction->OnEndCmd.IsValid()) pSOEntity->FireEvent(pSOAction->OnEndCmd, P);
		EventSrv->FireEvent(CStrID("OnSOActionEnd"), P);
	}
	else
	{
		if (pSOAction->ResetOnAbort()) pSOAction->Progress = 0.f;
		if (pSOAction->OnAbortCmd.IsValid()) pSOEntity->FireEvent(pSOAction->OnAbortCmd, P);
		EventSrv->FireEvent(CStrID("OnSOActionAbort"), P);
	}
}
//---------------------------------------------------------------------

bool CActionUseSmartObj::IsValid(CActor* pActor) const
{
	Game::CEntity* pSOEntity = EntityMgr->GetEntity(TargetID);
	if (!pSOEntity || pSOEntity->GetLevel() != pActor->GetEntity()->GetLevel()) FAIL;
	CPropSmartObject* pSO = pSOEntity->GetProperty<CPropSmartObject>();
	if (!pSO) FAIL;
	CSmartObjAction* pSOAction = pSO->GetAction(ActionID);
	return	pSOAction &&
			pSOAction->Enabled &&
			(WasDone || pSOAction->Resource) &&
			((SubActFace.IsValid() && SubActFace->IsValid(pActor)) ||
			 (!pSOAction->UpdateValidator.IsValid() || pSOAction->UpdateValidator->IsValid(pActor, pSO, pSOAction)));
}
//---------------------------------------------------------------------

}