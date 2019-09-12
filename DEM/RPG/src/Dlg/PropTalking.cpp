#include "PropTalking.h"

#include <Dlg/DialogueManager.h>
#include <Scripting/PropScriptable.h>
#include <AI/PropSmartObject.h>
#include <Game/Entity.h>
#include <Events/EventServer.h>
#include <Core/Factory.h>

namespace Prop
{
__ImplementClass(Prop::CPropTalking, 'PTLK', Game::CProperty);
__ImplementPropertyStorage(CPropTalking);

bool CPropTalking::InternalActivate()
{
	const CString& Dlg = GetEntity()->GetAttr<CString>(CStrID("Dialogue"), CString::Empty);
	if (Dlg.IsValid()) Dialogue = DlgMgr->GetDialogueGraph(CStrID(Dlg.CStr()));

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive()) EnableSI(*pProp);

	CPropSmartObject* pPropSO = GetEntity()->GetProperty<CPropSmartObject>();
	if (pPropSO && pPropSO->IsActive()) pPropSO->EnableAction(CStrID("Talk"));

	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropTalking, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnPropDeactivating, CPropTalking, OnPropDeactivating);
	PROP_SUBSCRIBE_PEVENT(OnSOActionStart, CPropTalking, OnSOActionStart);
	OK;
}
//---------------------------------------------------------------------

void CPropTalking::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnPropDeactivating);
	UNSUBSCRIBE_EVENT(OnSOActionStart);

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive()) DisableSI(*pProp);

	CPropSmartObject* pPropSO = GetEntity()->GetProperty<CPropSmartObject>();
	if (pPropSO && pPropSO->IsActive()) pPropSO->EnableAction(CStrID("Talk"), false);

	//???check IsTalking and force abort dlg?
	Dialogue = nullptr;
}
//---------------------------------------------------------------------

bool CPropTalking::OnPropActivated(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
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

bool CPropTalking::OnPropDeactivating(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
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

	P = n_new(Data::CParams(1));
	P->Set(CStrID("EntityID"), GetEntity()->GetUID());
	//!!!TODO: Calculate time
	EventSrv->ScheduleEvent(CStrID("HidePhrase"), P, 0, nullptr, 5.f);
}
//---------------------------------------------------------------------

bool CPropTalking::OnSOActionStart(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	CStrID ActionID = P->Get<CStrID>(CStrID("Action"));
	CStrID SOID = P->Get<CStrID>(CStrID("SO"));

	if (ActionID == CStrID("Talk") && SOID != GetEntity()->GetUID())
		DlgMgr->RequestDialogue(GetEntity()->GetUID(), SOID);

	OK;
}
//---------------------------------------------------------------------

}