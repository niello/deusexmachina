#include "SteeringController.h"
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CSteeringController, 'STCL', CTraversalController);

CStrID CSteeringController::FindAction(const CNavAgentComponent&, unsigned char, dtPolyRef, Game::HEntity*)
{
	return CStrID("Steer");
}
//---------------------------------------------------------------------

bool CSteeringController::PushSubAction(Game::CActionQueueComponent& Queue, const Events::CEventBase& ParentAction, CStrID Type,
	const vector3& Dest, const vector3& NextDest, float RemainingDistance, Game::HEntity)
{
	FAIL;
}
//---------------------------------------------------------------------

}
