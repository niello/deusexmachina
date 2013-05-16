#include "PropTalking.h"

#include <Story/Dlg/DialogueManager.h>
#include <Game/EntityManager.h>
#include <DB/DBServer.h>
#include <Events/EventManager.h>

namespace Attr
{
	DefineString(Dialogue);
};

BEGIN_ATTRS_REGISTRATION(PropTalking)
	RegisterString(Dialogue, ReadOnly);
END_ATTRS_REGISTRATION

namespace Properties
{
__ImplementClass(Properties::CPropTalking, 'PTLK', Game::CProperty);
__ImplementPropertyStorage(CPropTalking);

void CPropTalking::Activate()
{
	Game::CProperty::Activate();

	const nString& Dlg = GetEntity()->GetAttr<nString>(CStrID("Dialogue"));
	if (Dlg.IsValid()) Dialogue = DlgMgr->GetDialogue(Dlg);

	PROP_SUBSCRIBE_PEVENT(ExposeSI, CPropTalking, ExposeSI);
	PROP_SUBSCRIBE_PEVENT(Talk, CPropTalking, OnTalk);
}
//---------------------------------------------------------------------

void CPropTalking::Deactivate()
{
	UNSUBSCRIBE_EVENT(ExposeSI);
	UNSUBSCRIBE_EVENT(Talk);

	//???check IsTalking and force abort dlg?
	Dialogue = NULL;
	Game::CProperty::Deactivate();
}
//---------------------------------------------------------------------

void CPropTalking::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	Attrs.Append(Attr::Dialogue);
}
//---------------------------------------------------------------------

void CPropTalking::SayPhrase(CStrID PhraseID)
{
	PParams P = n_new(CParams);

	//!!! Try to find the phrase by it's ID before saying it.
	P->Set(CStrID("Text"), nString(PhraseID.CStr()));
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

} // namespace Properties