#include "PropTalking.h"

#include <Dlg/DialogueManager.h>
#include <Scripting/PropScriptable.h>
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

	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropTalking, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnPropDeactivating, CPropTalking, OnPropDeactivating);
	PROP_SUBSCRIBE_PEVENT(Talk, CPropTalking, OnTalk);
	OK;
}
//---------------------------------------------------------------------

void CPropTalking::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnPropDeactivating);
	UNSUBSCRIBE_EVENT(Talk);

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive()) DisableSI(*pProp);

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
	//!!! TODO: Calculate time
	EventSrv->FireEvent(CStrID("HidePhrase"), P, 0, 5.f);
}
//---------------------------------------------------------------------

// "Talk" command handler, starts dialogue of this entity and initiator
bool CPropTalking::OnTalk(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	DlgMgr->RequestDialogue(GetEntity()->GetUID(), P->Get<CStrID>(CStrID("Actor")));
	OK;
}
//---------------------------------------------------------------------

}