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
	// FIXME: is Dest change a good criteria for distance update? Move criteria to navigation?
	U8 Result = NeedDistanceToTarget;
	float Distance = -0.f;
	if (auto pAction = Queue.GetImmediateSubAction(ParentAction))
	{
		if (auto pSteer = pAction->As<Steer>())
		{
			if (pSteer->_Dest == Dest)
			{
				Result &= ~NeedDistanceToTarget;
				Distance = pSteer->_AdditionalDistance;
			}
		}
	}

	auto pSteer = Queue.PushSubActionForParent<Steer>(ParentAction, Dest, NextDest, Distance);
	return pSteer ? Result : Failure;
}
//---------------------------------------------------------------------

void CSteerAction::SetDistanceAfterDest(Game::CActionQueueComponent& Queue, const Events::CEventBase& ParentAction, float Distance)
{
	auto pAction = Queue.GetImmediateSubAction(ParentAction);
	if (auto pSteer = pAction->As<Steer>())
		pSteer->_AdditionalDistance = Distance;
}
//---------------------------------------------------------------------

}
