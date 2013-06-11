#include "PropTransitionZone.h"

#include <Game/EntityManager.h>
#include <Events/EventManager.h>

extern const matrix44 Rotate180(
	-1.f, 0.f,  0.f, 0.f,
	 0.f, 1.f,  0.f, 0.f,
	 0.f, 0.f, -1.f, 0.f,
	 0.f, 0.f,  0.f, 1.f);

//BEGIN_ATTRS_REGISTRATION(PropTransitionZone)
//	RegisterString(TargetLevelID, ReadOnly);
//	RegisterString(DestPoint, ReadOnly);
//END_ATTRS_REGISTRATION

namespace Prop
{
__ImplementClass(Prop::CPropTransitionZone, 'PRTZ', Game::CProperty);
__ImplementPropertyStorage(CPropTransitionZone);

bool CPropTransitionZone::InternalActivate()
{
	//???check level existence here?
	PROP_SUBSCRIBE_PEVENT(Travel, CPropTransitionZone, OnTravel);
	OK;
}
//---------------------------------------------------------------------

void CPropTransitionZone::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(Travel);
}
//---------------------------------------------------------------------

// "Travel" command handler
bool CPropTransitionZone::OnTravel(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CEntity* pActorEnt = EntityMgr->GetEntity(P->Get<CStrID>(CStrID("Actor")));
	n_assert(pActorEnt);

	const nString& LevelID = GetEntity()->GetAttr<nString>(CStrID("TargetLevelID"));
	const nString& DestPt = GetEntity()->GetAttr<nString>(CStrID("DestPoint"));

	//???set pause?

	if (DestPt.IsValid())
	{
		//matrix44 Tfm;
		//pActorEnt->Set<matrix44>(CStrID("Transform"), Rotate180 * Tfm); //???or fire SetTransform?
		//else n_printf("Travel, Warning: destination point '%s' not found\n", DestPt.CStr());
	}

	pActorEnt->SetAttr<nString>(CStrID("LevelID"), LevelID);
	
	//LoaderSrv->CommitChangesToDB();

	P = n_new(Data::CParams(1));
	P->Set(CStrID("LevelID"), LevelID);
	EventMgr->FireEvent(CStrID("RequestLevel"), P);

	OK;
}
//---------------------------------------------------------------------

} // namespace Prop
