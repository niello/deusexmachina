#pragma once
#include <Core/Object.h>
#include <Game/ECS/Entity.h>
#include <Events/EventBase.h>
#include <Math/Vector3.h>

// Controls a path edge traversal by providing appropriate action for the agent

#ifdef DT_POLYREF64
#error "64-bit navigation poly refs aren't supported for now"
#else
typedef unsigned int dtPolyRef;
#endif

namespace DEM::Game
{
	struct CActionQueueComponent;
}

namespace DEM::AI
{
struct CNavAgentComponent;
using PTraversalController = Ptr<class CTraversalController>;

class CTraversalController : public ::Core::CObject
{
	RTTI_CLASS_DECL;

protected:

	//

public:

	virtual CStrID FindAction(const CNavAgentComponent& Agent, unsigned char AreaType, dtPolyRef Poly, Game::HEntity* pOutSmartObject) = 0;
	virtual bool   PushSubAction(Game::CActionQueueComponent& Queue, const Events::CEventBase& ParentAction, CStrID Type,
		const vector3& Dest, const vector3& NextDest, float RemainingDistance, Game::HEntity SmartObject) = 0;
};

}
