#pragma once
#include <AI/Navigation/TraversalController.h>

// Provides steering traversal action, processed by the character controller
// or similar components. It is the most common case, simple movement.

namespace DEM::AI
{

class CSteeringController : public CTraversalController
{
	FACTORY_CLASS_DECL;

protected:

	//

public:

	virtual CStrID FindAction(const CNavAgentComponent& Agent, unsigned char AreaType, dtPolyRef Poly, Game::HEntity* pOutSmartObject) override;
	virtual bool   PushSubAction(Game::CActionQueueComponent& Queue, const Events::CEventBase& ParentAction, CStrID Type,
		const vector3& Dest, const vector3& NextDest, float RemainingDistance, Game::HEntity SmartObject) override;
};

}
