#pragma once
#include <Core/Object.h>
#include <Game/ECS/Entity.h>
#include <Events/EventBase.h>
#include <Math/Vector3.h>

// Controls a path edge traversal by providing appropriate action for the agent,
// e.g. Steer (Walk), Swim, Jump, OpenDoor, ClimbLadder etc.

#ifdef DT_POLYREF64
#error "64-bit navigation poly refs aren't supported for now"
#else
typedef unsigned int dtPolyRef;
#endif

namespace DEM::Game
{
	class CActionQueueComponent;
}

namespace DEM::AI
{
struct CNavAgentComponent;
using PTraversalController = Ptr<class CTraversalAction>;

class CTraversalAction : public ::Core::CObject
{
	RTTI_CLASS_DECL;

public:

	// FIXME: ugly API, but it is the best one I came up with to avoid recalculation of full distance to target
	// PushSubAction result flags
	enum
	{
		Failure              = 0x01,
		NeedDistanceToTarget = 0x02
	};

	virtual bool CanSkipPathPoint(float SqDistance) const = 0;
	virtual U8   PushSubAction(Game::CActionQueueComponent& Queue, const Events::CEventBase& ParentAction,
		const vector3& Dest, const vector3& NextDest, Game::HEntity SmartObject) = 0;
	virtual void SetDistanceAfterDest(Game::CActionQueueComponent& Queue, const Events::CEventBase& ParentAction, float Distance) {}
};

}