#include "PropTalking.h"

#include <Dlg/DialogueManager.h>
#include <Scripting/PropScriptable.h>
#include <AI/PropSmartObject.h>
#include <Game/EntityManager.h>
#include <Events/EventServer.h>

namespace Prop
{
__ImplementClass(Prop::CPropTalking, 'PTLK', Game::CProperty);
__ImplementPropertyStorage(CPropTalking);

bool CPropTalking::InternalActivate()
{
	const CString& Dlg = GetEntity()->GetAttr<CString>(CStrID("Dialogue"), NULL);
	if (Dlg.IsValid()) Dialogue = DlgMgr->GetDialogueGraph(CStrID(Dlg.CStr()));

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive()) EnableSI(*pProp);

	CPropSmartObject* pPropSO = GetEntity()->GetProperty<CPropSmartObject>();
	if (pPropSO && pPropSO->IsActive()) pPropSO->EnableAction(CStrID("Talk"));

	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropTalking, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnPropDeactivating, CPropTalking, OnPropDeactivating);
	PROP_SUBSCRIBE_PEVENT(OnSOActionStart, CPropTalking, OnSOActionStart);
	PROP_SUBSCRIBE_PEVENT(OnDlgRequested, CPropTalking, OnDlgRequested);
	OK;
}
//---------------------------------------------------------------------

void CPropTalking::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnPropDeactivating);
	UNSUBSCRIBE_EVENT(OnSOActionStart);
	UNSUBSCRIBE_EVENT(OnDlgRequested);

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive()) DisableSI(*pProp);

	CPropSmartObject* pPropSO = GetEntity()->GetProperty<CPropSmartObject>();
	if (pPropSO && pPropSO->IsActive()) pPropSO->EnableAction(CStrID("Talk"), false);

	//???check IsTalking and force abort dlg?
	Dialogue = NULL;
}
//---------------------------------------------------------------------

bool CPropTalking::OnPropActivated(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropScriptable>())
	{
		EnableSI(*(CPropScriptable*)pProp);
		OK;
	}

	if (pProp->IsA<CPropSmartObject>())
	{
		((CPropSmartObject*)pProp)->EnableAction(CStrID("Talk"));
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropTalking::OnPropDeactivating(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropScriptable>())
	{
		DisableSI(*(CPropScriptable*)pProp);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

void CPropTalking::SayPhrase(CStrID PhraseID)
{
	Data::PParams P = n_new(Data::CParams);

	//!!! Try to find the phrase by it's ID before saying it.
	P->Set(CStrID("Text"), CString(PhraseID.CStr()));
	P->Set(CStrID("EntityID"), GetEntity()->GetUID());
	EventSrv->FireEvent(CStrID("ShowPhrase"), P);

	P = n_new(Data::CParams);
	P->Set(CStrID("EntityID"), GetEntity()->GetUID());
	//!!!TODO: Calculate time
	EventSrv->FireEvent(CStrID("HidePhrase"), P, 0, 5.f);
}
//---------------------------------------------------------------------

bool CPropTalking::OnSOActionStart(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	CStrID ActionID = P->Get<CStrID>(CStrID("Action"));

	if (ActionID == CStrID("Talk"))
		DlgMgr->RequestDialogue(GetEntity()->GetUID(), P->Get<CStrID>(CStrID("SO")));

	OK;
}
//---------------------------------------------------------------------

bool CPropTalking::OnDlgRequested(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;

	//!!!can script it in plr/npc classes!
	//facing initiator, breaking least prioritized dialogue is here
	DlgMgr->AcceptDialogue(P->Get<CStrID>(CStrID("Initiator")), GetEntity()->GetUID());

	OK;
}
//---------------------------------------------------------------------

}