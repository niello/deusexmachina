#include "ActionUseSmartObj.h"

#include <AI/PropActorBrain.h>
#include <AI/PropSmartObject.h>
#include <Game/GameServer.h>
#include <Core/Factory.h>

namespace AI
{
__ImplementClass(AI::CActionUseSmartObj, 'AUSO', AI::CAction)

UPTR CActionUseSmartObj::SetDone(CActor* pActor, Prop::CPropSmartObject* pSO, const CSmartAction& ActTpl)
{
	WasDone = true;

	Data::PParams P = n_new(Data::CParams(3));
	P->Set(CStrID("Actor"), pActor->GetEntity()->GetUID());
	P->Set(CStrID("SO"), TargetID);
	P->Set(CStrID("Action"), ActionID);
	pActor->GetEntity()->FireEvent(CStrID("OnSOActionDone"), P);
	pSO->GetEntity()->FireEvent(CStrID("OnSOActionDone"), P);

	return ActTpl.EndOnDone() ? Success : Running;
}
//---------------------------------------------------------------------

bool CActionUseSmartObj::Activate(CActor* pActor)
{
	Game::CEntity* pSOEntity = GameSrv->GetEntityMgr()->GetEntity(TargetID);
	if (!pSOEntity || pSOEntity->GetLevel() != pActor->GetEntity()->GetLevel()) FAIL;
	Prop::CPropSmartObject* pSO = pSOEntity->GetProperty<Prop::CPropSmartObject>();
	if (!pSO || !pSO->IsActionAvailable(ActionID, pActor)) FAIL;
	Prop::CPropSmartObject::CAction* pSOAction = pSO->GetAction(ActionID);
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
	else Sys::Error("CActionUseSmartObj::StartSOAction(): Unknown ProgressDriver!");

	Prop::CPropSmartObject* pActorSO = pActor->GetEntity()->GetProperty<Prop::CPropSmartObject>();
	if (pActorSO)
		pActorSO->SetState(CStrID("UsingSO"), ActionID); //!!!SYNC_ACTOR_ANIMATION ? Duration : -1.f

	Data::PParams P = n_new(Data::CParams(3));
	P->Set(CStrID("Actor"), pActor->GetEntity()->GetUID());
	P->Set(CStrID("SO"), TargetID);
	P->Set(CStrID("Action"), ActionID);
	pActor->GetEntity()->FireEvent(CStrID("OnSOActionStart"), P);
	pSOEntity->FireEvent(CStrID("OnSOActionStart"), P);

	if (Duration == 0.f)
	{
		WasDone = true;
		pActor->GetEntity()->FireEvent(CStrID("OnSOActionDone"), P);
		pSOEntity->FireEvent(CStrID("OnSOActionDone"), P);
	}

	OK;
}
//---------------------------------------------------------------------

UPTR CActionUseSmartObj::Update(CActor* pActor)
{
	//!!!IsValid() checks that values, second test may be not necessary!
	Game::CEntity* pSOEntity = GameSrv->GetEntityMgr()->GetEntity(TargetID);
	if (!pSOEntity || pSOEntity->GetLevel() != pActor->GetEntity()->GetLevel()) return Failure;
	Prop::CPropSmartObject* pSO = pSOEntity->GetProperty<Prop::CPropSmartObject>();
	if (!pSO) return Failure;
	Prop::CPropSmartObject::CAction* pSOAction = pSO->GetAction(ActionID);
	if (!pSOAction) return Failure;
	const CSmartAction& ActTpl = *pSOAction->pTpl;

	UPTR Result = ActTpl.Update(pActor->GetEntity()->GetUID(), TargetID);
	if (Result == Failure || ExecResultIsError(Result)) return Result;
	if (WasDone) return ActTpl.EndOnDone() ? Success : Running;
	if (Result == Success) return SetDone(pActor, pSO, ActTpl);
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

		if (IsDone) return SetDone(pActor, pSO, ActTpl);
		else if (ActTpl.SendProgressEvent() && Progress != PrevProgress)
		{
			Data::PParams P = n_new(Data::CParams(5));
			P->Set(CStrID("Actor"), pActor->GetEntity()->GetUID());
			P->Set(CStrID("SO"), TargetID);
			P->Set(CStrID("Action"), ActionID);
			P->Set(CStrID("PrevValue"), PrevProgress);
			P->Set(CStrID("Value"), Progress);
			pActor->GetEntity()->FireEvent(CStrID("OnSOActionProgress"), P);
			pSOEntity->FireEvent(CStrID("OnSOActionProgress"), P);
		}
	}

	return Running;
}
//---------------------------------------------------------------------

void CActionUseSmartObj::Deactivate(CActor* pActor)
{
	Game::CEntity* pSOEntity = GameSrv->GetEntityMgr()->GetEntity(TargetID);
	if (!pSOEntity || pSOEntity->GetLevel() != pActor->GetEntity()->GetLevel()) return;
	Prop::CPropSmartObject* pSO = pSOEntity->GetProperty<Prop::CPropSmartObject>();
	if (!pSO) return;
	Prop::CPropSmartObject::CAction* pSOAction = pSO->GetAction(ActionID);
	if (!pSOAction) return;
	const CSmartAction& ActTpl = *pSOAction->pTpl;

	Data::PParams P = n_new(Data::CParams(3));
	P->Set(CStrID("Actor"), pActor->GetEntity()->GetUID());
	P->Set(CStrID("SO"), TargetID);
	P->Set(CStrID("Action"), ActionID);

	if (WasDone)
	{
		pActor->GetEntity()->FireEvent(CStrID("OnSOActionEnd"), P);
		pSOEntity->FireEvent(CStrID("OnSOActionEnd"), P);
		if (ActTpl.ProgressDriver == CSmartAction::PDrv_Duration && !ActTpl.ResetOnAbort())
		{
			CString AttrID("ActionProgress_");
			AttrID += ActionID.CStr();
			pActor->GetEntity()->DeleteAttr(CStrID(AttrID.CStr()));
		}
	}
	else
	{
		pActor->GetEntity()->FireEvent(CStrID("OnSOActionAbort"), P);
		pSOEntity->FireEvent(CStrID("OnSOActionAbort"), P);
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
	Prop::CPropSmartObject* pActorSO = pActor->GetEntity()->GetProperty<Prop::CPropSmartObject>();
	if (pActorSO && pActorSO->GetCurrState() == CStrID("UsingSO"))
		pActorSO->SetState(CStrID("Idle"), ActionID);
	//???if in transition to UsingSO, reset to Idle too?

	if (pSOAction->FreeUserSlots >= 0) ++pSOAction->FreeUserSlots;
}
//---------------------------------------------------------------------

bool CActionUseSmartObj::IsValid(CActor* pActor) const
{
	Game::CEntity* pSOEntity = GameSrv->GetEntityMgr()->GetEntity(TargetID);
	if (!pSOEntity || pSOEntity->GetLevel() != pActor->GetEntity()->GetLevel()) FAIL;
	Prop::CPropSmartObject* pSO = pSOEntity->GetProperty<Prop::CPropSmartObject>();
	return pSO && pSO->IsActionEnabled(ActionID);
}
//---------------------------------------------------------------------

}