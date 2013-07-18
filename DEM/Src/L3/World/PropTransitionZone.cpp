#include "PropTransitionZone.h"

#include <World/WorldManager.h>
#include <Game/GameServer.h>
#include <Events/EventServer.h>

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

	//!!!if party or smth, collect a group of travellers!
	CArray<CStrID> TravellerIDs;
	TravellerIDs.Add(pActorEnt->GetUID());

	//???do request here in all cases and let application to call MakeTransition?
	//!!!include GameServer.h won't be needed

	// Request params:
	// - entitiy list
	// - dest level
	// - dest marker
	// - is far travel
	// - what levels to unload (none / specified / all departure levels / all loaded levels)
	//   or it is decided by app based on far flag (near - none, far - all loaded)

		//!!!DBG uncomment or move to app, see above!
	//if (IsFarTravel)
	//{
		//test all travellers to be in range / colliding with transition zone shape
		//!!!when check distance from party members to object (if not by collision), check if level is the same! 
		//request
	//}
	//else
	//{
		//!!!DBG uncomment or move to app, see above!
		//if (GameSrv->IsLevelLoaded(LevelID))
			WorldMgr->MakeTransition(TravellerIDs, pActorEnt->GetLevel().GetID(), LevelID, MarkerID, false);

			//!!!not here!
			GameSrv->SetActiveLevel(LevelID);
		//else
		//{
			//request
		//}
	//}

	//!!!stop game timer when in loading state!

	//P = n_new(Data::CParams(1));
	//P->Set(CStrID("LevelID"), LevelID);
	//EventSrv->FireEvent(CStrID("RequestLevel"), P);

	OK;
}
//---------------------------------------------------------------------

}