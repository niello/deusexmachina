#include "PropTransitionZone.h"

#include <Game/EntityManager.h>
#include <Events/EventServer.h>
#include <Data/DataArray.h>

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
	if (!pActorEnt) FAIL;

	CStrID LevelID = GetEntity()->GetAttr<CStrID>(CStrID("DestLevelID"));
	CStrID MarkerID = GetEntity()->GetAttr<CStrID>(CStrID("DestMarkerID"));
	bool IsFarTravel = GetEntity()->GetAttr<bool>(CStrID("FarTravel"), false);
	n_assert(LevelID.IsValid() && MarkerID.IsValid());

//=========================
	//???wait until all entities are in zone, re-calling Travel action when zone shape
	// is intersected by a new character, and fire event only when transition can really be performed?
	//need to implement such a continuous action mechanism.

	//???wait group here or in prop? in prop is more logical, here we don't know
	// anything about transition zone and its shape

	//!!!if party, collect a group of travellers! for far travel, all party,
	// for near - selection or wait for all selection is in a zone
	Data::PDataArray IDs = n_new(Data::CDataArray(1, 5));
	IDs->Add(pActorEnt->GetUID());

	if (IsFarTravel)
	{
		//test all travellers to be in range / colliding with transition zone shape
		//!!!when check distance from party members to object (if not by collision), check if level is the same! 
	}
	else
	{
		//???wait until all selected entities are in range of transition zone?
		//???or this logic must be in zone?
	}
//=========================

	// This property doesn't invoke actual transition, it notifies application instead.
	// So, application can decide how to handle transition request.
	P = n_new(Data::CParams(4));
	P->Set(CStrID("EntityIDs"), IDs);
	P->Set(CStrID("LevelID"), LevelID);
	P->Set(CStrID("MarkerID"), MarkerID);
	P->Set(CStrID("IsFarTravel"), IsFarTravel);
	return (EventSrv->FireEvent(CStrID("OnWorldTransitionRequested"), P) > 0);
}
//---------------------------------------------------------------------

}