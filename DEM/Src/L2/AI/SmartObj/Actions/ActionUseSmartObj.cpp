#include "ActionUseSmartObj.h"

#include <AI/PropActorBrain.h>
#include <AI/PropSmartObject.h>
#include <Game/EntityManager.h>
#include <Game/GameServer.h>
#include <Events/EventServer.h>

namespace AI
{
__ImplementClass(AI::CActionUseSmartObj, 'AUSO', AI::CAction)

using namespace Prop;
using namespace Data;

DWORD CActionUseSmartObj::SetDone(CActor* pActor, const CSmartAction& ActTpl)
{
	WasDone = true;

	PParams P = n_new(CParams(2));
	//P->Set(CStrID("Actor"), pActor->GetEntity()->GetUID());
	P->Set(CStrID("SO"), TargetID);
	P->Set(CStrID("Action"), ActionID);
	pActor->GetEntity()->FireEvent(CStrID("OnSOActionDone"), P);

	return ActTpl.EndOnDone() ? Success : Running;
}
//---------------------------------------------------------------------

bool CActionUseSmartObj::Activate(CActor* pActor)
{
	Game::CEntity* pSOEntity = EntityMgr->GetEntity(TargetID);
	if (!pSOEntity || pSOEntity->GetLevel() != pActor->GetEntity()->GetLevel()) FAIL;
	CPropSmartObject* pSO = pSOEntity->GetProperty<CPropSmartObject>();
	if (!pSO || !pSO->IsActionAvailable(ActionID, pActor)) FAIL;
	CPropSmartObject::CAction* pSOAction = pSO->GetAction(ActionID);
	const CSmartAction& ActTpl = *pSOAction->pTpl;

	WasDone = false;

	if (pSOAction->FreeUserSlots > 0) --pSOAction->FreeUserSlots;

	if (ActTpl.ProgressDriver == CSmartAction::PDrv_Duration)
	{
		CString AttrID("ActionProgress_");
		AttrID += ActionID.CStr();

		Duration = ActTpl.GetDuration(pActor->GetEntity()->GetUID(), TargetID);
		Progress = ActTpl.ResetOnAbort() ? 0.f : pActor->GetEntity()->GetAttr<float>(CStrID(AttrID.CStr()), 0.f);

		if (ActTpl.TargetState.IsValid())
		{
			pSO->SetState(ActTpl.TargetState, ActionID, Duration, ActTpl.ManualTransitionControl());
			pSO->SetTransitionProgress(Progress);
		}
	}
	else if (ActTpl.ProgressDriver == CSmartAction::PDrv_SO_FSM)
	{
		n_assert_dbg(ActTpl.TargetState.IsValid());
		pSO->SetState(ActTpl.TargetState, ActionID);
		Duration = pSO->GetTransitionDuration();
		Progress = pSO->GetTransitionProgress();
	}
	else if (ActTpl.ProgressDriver == CSmartAction::PDrv_None)
	{
		//???if _TARGET_STATE
		//???	SO.FSM.SetState(_TARGET_STATE, _ACTION_ID, Auto)
		Duration = -1.f;
	}
	else Core::Error("CActionUseSmartObj::StartSOAction(): Unknown ProgressDriver!");

	CPropSmartObject* pActorSO = pActor->GetEntity()->GetProperty<CPropSmartObject>();
	if (pActorSO)
		pActorSO->SetState(CStrID("UsingSO"), ActionID); //!!!SYNC_ACTOR_ANIMATION ? Duration : -1.f

	PParams P = n_new(CParams(2)); //3));
	//P->Set(CStrID("Actor"), pActor->GetEntity()->GetUID());
	P->Set(CStrID("SO"), TargetID);
	P->Set(CStrID("Action"), ActionID);
	pActor->GetEntity()->FireEvent(CStrID("OnSOActionStart"), P);

	if (Duration == 0.f)
	{
		WasDone = true;
		pActor->GetEntity()->FireEvent(CStrID("OnSOActionDone"), P);
	}

	OK;
}
//---------------------------------------------------------------------

DWORD CActionUseSmartObj::Update(CActor* pActor)
{
	//!!!IsValid() checks that values, second test may be not necessary!
	Game::CEntity* pSOEntity = EntityMgr->GetEntity(TargetID);
	if (!pSOEntity || pSOEntity->GetLevel() != pActor->GetEntity()->GetLevel()) return Failure;
	CPropSmartObject* pSO = pSOEntity->GetProperty<CPropSmartObject>();
	if (!pSO) return Failure;
	CPropSmartObject::CAction* pSOAction = pSO->GetAction(ActionID);
	if (!pSOAction) return Failure;
	const CSmartAction& ActTpl = *pSOAction->pTpl;

	DWORD Result = Running; //!!!if UpdateFunc() call it!
	if (Result == Failure || ExecResultIsError(Result)) return Result;
	if (WasDone) return ActTpl.EndOnDone() ? Success : Running;
	if (Result == Success) return SetDone(pActor, ActTpl);
	else if (ActTpl.ProgressDriver != CSmartAction::PDrv_None)
	{
		bool IsDone = false;
		float PrevProgress = Progress;
		if (ActTpl.ProgressDriver == CSmartAction::PDrv_Duration)
		{
			Progress += (float)GameSrv->GetFrameTime();
			if (ActTpl.TargetState.IsValid() && ActTpl.ManualTransitionControl())
				pSO->SetTransitionProgress(Progress);
			IsDone = (Progress >= Duration);
		}
		else if (ActTpl.ProgressDriver == CSmartAction::PDrv_SO_FSM)
		{
			if (pSO->IsInTransition())
			{
				if (pSO->GetTargetState() != ActTpl.TargetState || pSO->GetTransitionActionID() != ActionID) return Failure;
				Progress = pSO->GetTransitionProgress();
			}
			else
			{
				if (pSO->GetCurrState() != ActTpl.TargetState) return Failure;
				IsDone = true;
			}
		}

		if (IsDone) return SetDone(pActor, ActTpl);
		else if (ActTpl.SendProgressEvent() && Progress != PrevProgress)
		{
			PParams P = n_new(CParams(4));
			//P->Set(CStrID("Actor"), pActor->GetEntity()->GetUID());
			P->Set(CStrID("SO"), TargetID);
			P->Set(CStrID("Action"), ActionID);
			P->Set(CStrID("PrevValue"), PrevProgress);
			P->Set(CStrID("Value"), Progress);
			pActor->GetEntity()->FireEvent(CStrID("OnSOActionProgress"), P);
		}
	}

	return Running;
}
//---------------------------------------------------------------------

void CActionUseSmartObj::Deactivate(CActor* pActor)
{
	Game::CEntity* pSOEntity = EntityMgr->GetEntity(TargetID);
	if (!pSOEntity || pSOEntity->GetLevel() != pActor->GetEntity()->GetLevel()) return;
	CPropSmartObject* pSO = pSOEntity->GetProperty<CPropSmartObject>();
	if (!pSO) return;
	CPropSmartObject::CAction* pSOAction = pSO->GetAction(ActionID);
	if (!pSOAction) return;
	const CSmartAction& ActTpl = *pSOAction->pTpl;

	PParams P = n_new(CParams(2));
	//P->Set(CStrID("Actor"), pActor->GetEntity()->GetUID());
	P->Set(CStrID("SO"), TargetID);
	P->Set(CStrID("Action"), ActionID);

	if (WasDone)
	{
		EventSrv->FireEvent(CStrID("OnSOActionEnd"), P);
		if (ActTpl.ProgressDriver == CSmartAction::PDrv_Duration && !ActTpl.ResetOnAbort())
		{
			CString AttrID("ActionProgress_");
			AttrID += ActionID.CStr();
			pActor->GetEntity()->DeleteAttr(CStrID(AttrID.CStr()));
		}
	}
	else
	{
		EventSrv->FireEvent(CStrID("OnSOActionAbort"), P);
		if (ActTpl.ProgressDriver == CSmartAction::PDrv_Duration && !ActTpl.ResetOnAbort())
		{
			CString AttrID("ActionProgress_");
			AttrID += ActionID.CStr();
			pActor->GetEntity()->SetAttr(CStrID(AttrID.CStr()), Progress);
		}
		if (ActTpl.TargetState.IsValid())
		{
			//if _RESET_ON_ABORT [? and drv is so fsm or (_SO_TIMING_MODE = Manual and _FREE_SLOTS = _MAX_SLOTS - 1)?]
			if (ActTpl.ResetOnAbort())
				pSO->AbortTransition(); //[?time/speed, not to switch anim immediately but to perform reverse transition?]
			else if (ActTpl.ProgressDriver == CSmartAction::PDrv_SO_FSM)
				pSO->StopTransition();
		}
	}

	// Abort might be caused by an actor state change, so don't affect that change
	CPropSmartObject* pActorSO = pActor->GetEntity()->GetProperty<CPropSmartObject>();
	if (pActorSO && pActorSO->GetCurrState() == CStrID("UsingSO"))
		pActorSO->SetState(CStrID("Idle"), ActionID);
	//???if in transition to UsingSO, reset to Idle too?

	if (pSOAction->FreeUserSlots >= 0) ++pSOAction->FreeUserSlots;
}
//---------------------------------------------------------------------

bool CActionUseSmartObj::IsValid(CActor* pActor) const
{
	Game::CEntity* pSOEntity = EntityMgr->GetEntity(TargetID);
	if (!pSOEntity || pSOEntity->GetLevel() != pActor->GetEntity()->GetLevel()) FAIL;
	CPropSmartObject* pSO = pSOEntity->GetProperty<CPropSmartObject>();
	return pSO && pSO->IsActionEnabled(ActionID);
}
//---------------------------------------------------------------------

}