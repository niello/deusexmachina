#include "SteeringController.h"
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <Core/Factory.h>

namespace DEM::AI
{
RTTI_CLASS_IMPL(Steer, Events::CEventNative);
RTTI_CLASS_IMPL(Turn, Events::CEventNative);
FACTORY_CLASS_IMPL(DEM::AI::CSteeringController, 'STCL', CTraversalController);

CStrID CSteeringController::FindAction(const CNavAgentComponent&, unsigned char, dtPolyRef, Game::HEntity*)
{
	// Only Steer action is supported by this controller
	static const CStrID sidSteer("Steer");
	return sidSteer;
}
//---------------------------------------------------------------------

bool CSteeringController::PushSubAction(Game::CActionQueueComponent& Queue, const Events::CEventBase& ParentAction, CStrID Type,
	const vector3& Dest, const vector3& NextDest, float RemainingDistance, Game::HEntity)
{
	static const CStrID sidSteer("Steer");
	if (Type != sidSteer) return false;

	if (auto pSteer = Queue.RequestSubAction<Steer>(ParentAction))
	{
		pSteer->_Dest = Dest;
		pSteer->_NextDest = NextDest;
		pSteer->_AdditionalDistance = RemainingDistance;
	}
	else
	{
		// TODO: could use pool
		Queue.Stack.push_back(Events::PEventBase(n_new(Steer(Dest, NextDest, RemainingDistance))));
	}

	return true;
}
//---------------------------------------------------------------------

}
