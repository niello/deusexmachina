#include "SteerAction.h"
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <Core/Factory.h>

namespace DEM::AI
{
RTTI_CLASS_IMPL(Steer, Events::CEventNative);
RTTI_CLASS_IMPL(Turn, Events::CEventNative);
FACTORY_CLASS_IMPL(DEM::AI::CSteerAction, 'STCL', CTraversalAction);

U8 CSteerAction::PushSubAction(Game::CActionQueueComponent& Queue, const Events::CEventBase& ParentAction,
	const vector3& Dest, const vector3& NextDest, Game::HEntity)
{
	Steer* pSteer;
	if (!Queue.RequestSubAction(ParentAction, pSteer)) return Failure;

	U8 Result = 0;
	if (pSteer)
	{
		if (pSteer->_Dest != Dest)
		{
			pSteer->_Dest = Dest;
			Result |= NeedDistanceToTarget;
		}
		pSteer->_NextDest = NextDest;
	}
	else
	{
		// TODO: could use pool
		Queue.Stack.push_back(Events::PEventBase(n_new(Steer(Dest, NextDest, -0.f))));
		Result |= NeedDistanceToTarget;
	}

	Queue.Status = DEM::Game::EActionStatus::InProgress;

	return Result;
}
//---------------------------------------------------------------------

void CSteerAction::SetDistanceToTarget(Game::CActionQueueComponent& Queue, const Events::CEventBase& ParentAction, float Distance)
{
	Steer* pSteer;
	if (Queue.RequestSubAction(ParentAction, pSteer) && pSteer)
		pSteer->_AdditionalDistance = Distance;
}
//---------------------------------------------------------------------

}
