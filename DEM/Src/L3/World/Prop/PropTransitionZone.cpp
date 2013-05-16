#include "PropTransitionZone.h"

#include <Game/EntityManager.h>
#include <Loading/LoaderServer.h>
#include <DB/DBServer.h>
#include <Events/EventManager.h>

extern const matrix44 Rotate180;

namespace Attr
{
	DeclareAttr(Transform);
	DeclareAttr(LevelID);

	DefineString(TargetLevelID);
	DefineString(DestPoint);
};

BEGIN_ATTRS_REGISTRATION(PropTransitionZone)
	RegisterString(TargetLevelID, ReadOnly);
	RegisterString(DestPoint, ReadOnly);
END_ATTRS_REGISTRATION

namespace Properties
{
__ImplementClass(Properties::CPropTransitionZone, 'PRTZ', Game::CProperty);
__ImplementPropertyStorage(CPropTransitionZone);

void CPropTransitionZone::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	Attrs.Append(Attr::TargetLevelID);
	Attrs.Append(Attr::DestPoint);
}
//---------------------------------------------------------------------

void CPropTransitionZone::Activate()
{
	Game::CProperty::Activate();
	//???check level existence here?
	PROP_SUBSCRIBE_PEVENT(Travel, CPropTransitionZone, OnTravel);
}
//---------------------------------------------------------------------

void CPropTransitionZone::Deactivate()
{
	UNSUBSCRIBE_EVENT(Travel);
	Game::CProperty::Deactivate();
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
		//if (EntityFct->GetEntityAttribute<matrix44>(DestPt, CStrID("Transform"), Tfm))
		//	pActorEnt->Set<matrix44>(CStrID("Transform"), Rotate180 * Tfm); //???or fire SetTransform?
		//else n_printf("Travel, Warning: destination point '%s' not found\n", DestPt.CStr());
	}

	pActorEnt->SetAttr<nString>(CStrID("LevelID"), LevelID);
	
	LoaderSrv->CommitChangesToDB();

	P = n_new(Data::CParams(1));
	P->Set(CStrID("LevelID"), LevelID);
	EventMgr->FireEvent(CStrID("RequestLevel"), P);

	OK;
}
//---------------------------------------------------------------------

} // namespace Properties
