#include "ActionUseSmartObj.h"

#include <AI/Prop/PropActorBrain.h>
#include <AI/Prop/PropSmartObject.h>
#include <AI/Movement/Actions/ActionFace.h>
#include <Game/Mgr/EntityManager.h>
#include <Game/GameServer.h>
#include <Events/EventManager.h>

namespace Attr
{
	DeclareAttr(Transform);
}

namespace AI
{
ImplementRTTI(AI::CActionUseSmartObj, AI::CAction)
ImplementFactory(AI::CActionUseSmartObj);

using namespace Properties;

void CActionUseSmartObj::StartSOAction(CActor* pActor)
{
	PParams P = n_new(CParams);
	P->Set(CStrID("Actor"), pActor->GetEntity()->GetUID());
	P->Set(CStrID("IAO"), TargetID);
	P->Set(CStrID("Action"), ActionID);
	if (Action->OnStartCmd.IsValid()) pSO->GetEntity()->FireEvent(Action->OnStartCmd, P);
	EventMgr->FireEvent(CStrID("OnIAOActionStart"), P);

	//!!!play anim for actor and for SO, if it has one for this action!

	if (Action->FreeUserSlots > 0) Action->FreeUserSlots--;
}
//---------------------------------------------------------------------

bool CActionUseSmartObj::Activate(CActor* pActor)
{
	Game::CEntity* pSOEntity = EntityMgr->GetEntityByID(TargetID);
	if (!pSOEntity) FAIL;

	pSO = pSOEntity->FindProperty<CPropSmartObject>();
	n_assert(pSO);
	Action = pSO->GetAction(ActionID);

	if (!Action->IsValid(pActor, pSO)) FAIL;

	WasDone = false;

	if (Action->FaceObject())
	{
		vector3 FaceDir = pSO->GetEntity()->Get<matrix44>(Attr::Transform).pos_component() - pActor->Position;
		FaceDir.norm();
		pActor->GetMotorSystem().SetFaceDirection(FaceDir);
		SubActFace = n_new(CActionFace);
		return SubActFace->Activate(pActor);
	}
	else StartSOAction(pActor);

	OK;
}
//---------------------------------------------------------------------

EExecStatus CActionUseSmartObj::Update(CActor* pActor)
{
	// Finish SO facing if started
	if (SubActFace.isvalid())
	{
		EExecStatus Status = SubActFace->Update(pActor);
		switch (Status)
		{
			case Running: return Running;
			case Success:
				SubActFace->Deactivate(pActor);
				SubActFace = NULL;
				StartSOAction(pActor);
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

	Action->Progress += (float)GameSrv->GetFrameTime();

	float Duration = Action->GetTpl().Duration;
	if (Duration >= 0.f && Action->Progress >= Duration)
	{
		if (Action->Resource > 0) Action->Resource--;

		WasDone = true;
		Action->Progress = 0.f;

		// SmartObj can be destroyed on action done, so cache this value
		bool EndOnDone = Action->EndOnDone();

		PParams P = n_new(CParams);
		P->Set(CStrID("Actor"), pActor->GetEntity()->GetUID());
		P->Set(CStrID("IAO"), TargetID);
		P->Set(CStrID("Action"), ActionID);
		if (Action->OnDoneCmd.IsValid()) pSO->GetEntity()->FireEvent(Action->OnDoneCmd, P);
		EventMgr->FireEvent(CStrID("OnIAOActionDone"), P);

		if (EndOnDone) return Success;
	}
	//else
		//???need to trigger some script function or fire OnActionProgressChanged event in certain time steps?

	return Running;
}
//---------------------------------------------------------------------

void CActionUseSmartObj::Deactivate(CActor* pActor)
{
	//???or unset when actor face SO first time, and then begin anim Progress?
	if (pActor->FacingStatus == AIFacing_DirSet)
		pActor->GetMotorSystem().ResetRotation();

	Game::CEntity* pSOEntity = EntityMgr->GetEntityByID(TargetID);
	if (!pSOEntity) return;

	if (Action->FreeUserSlots >= 0) Action->FreeUserSlots++;
	
	PParams P = n_new(CParams);
	P->Set(CStrID("Actor"), pActor->GetEntity()->GetUID());
	P->Set(CStrID("IAO"), TargetID);
	P->Set(CStrID("Action"), ActionID);

	if (WasDone)
	{
		//???GetEntity()->FireEvent(CStrID("PathAnimStop"));?
		if (Action->OnEndCmd.IsValid()) pSOEntity->FireEvent(Action->OnEndCmd, P);
		EventMgr->FireEvent(CStrID("OnIAOActionEnd"), P);
	}
	else
	{
		if (Action->ResetOnAbort()) Action->Progress = 0.f;
		if (Action->OnAbortCmd.IsValid()) pSOEntity->FireEvent(Action->OnAbortCmd, P);
		EventMgr->FireEvent(CStrID("OnIAOActionAbort"), P);
	}
}
//---------------------------------------------------------------------

bool CActionUseSmartObj::IsValid(CActor* pActor) const
{
	return	EntityMgr->ExistsEntityByID(TargetID) &&
			Action->Enabled &&
			(WasDone || Action->Resource) &&
			((SubActFace.isvalid() && SubActFace->IsValid(pActor)) ||
			 (!Action->UpdateValidator.isvalid() || Action->UpdateValidator->IsValid(pActor, pSO, Action)));
}
//---------------------------------------------------------------------

} //namespace AI