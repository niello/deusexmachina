#include "PropTalking.h"

#include <Dlg/DialogueManager.h>
#include <Game/EntityManager.h>
#include <Events/EventManager.h>

namespace Prop
{
__ImplementClass(Prop::CPropTalking, 'PTLK', Game::CProperty);
__ImplementPropertyStorage(CPropTalking);

bool CPropTalking::InternalActivate()
{
	const CString& Dlg = GetEntity()->GetAttr<CString>(CStrID("Dialogue"), NULL);
	if (Dlg.IsValid()) Dialogue = DlgMgr->GetDialogue(Dlg);

	PROP_SUBSCRIBE_PEVENT(ExposeSI, CPropTalking, ExposeSI);
	PROP_SUBSCRIBE_PEVENT(Talk, CPropTalking, OnTalk);

	OK;
}
//---------------------------------------------------------------------

void CPropTalking::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(ExposeSI);
	UNSUBSCRIBE_EVENT(Talk);

	//???check IsTalking and force abort dlg?
	Dialogue = NULL;
}
//---------------------------------------------------------------------

void CPropTalking::SayPhrase(CStrID PhraseID)
{
	PParams P = n_new(CParams);

	//!!! Try to find the phrase by it's ID before saying it.
	P->Set(CStrID("Text"), CString(PhraseID.CStr()));
	P->Set(CStrID("EntityID"), GetEntity()->GetUID());
	EventMgr->FireEvent(CStrID("ShowPhrase"), P);

	P = n_new(CParams);
	P->Set(CStrID("EntityID"), GetEntity()->GetUID());
	//!!! TODO: Calculate time
	EventMgr->FireEvent(CStrID("HidePhrase"), P, 0, 5.f);
}
//---------------------------------------------------------------------

// "Talk" command handler, starts dialogue of this entity and initiator
bool CPropTalking::OnTalk(const Events::CEventBase& Event)
{
	PParams P = ((const Events::CEvent&)Event).Params;
	Game::CEntity* pActorEnt = EntityMgr->GetEntity(P->Get<CStrID>(CStrID("Actor"), CStrID::Empty));
	DlgMgr->StartDialogue(GetEntity(), pActorEnt, true);
	OK;
}
//---------------------------------------------------------------------

} // namespace Prop