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
using PTraversalAction = Ptr<class CTraversalAction>;
struct CNavAgentComponent;
class Navigate;

class CTraversalAction : public ::Core::CObject
{
	RTTI_CLASS_DECL;

public:

	virtual float GetSqTriggerRadius(float AgentRadius) const = 0;
	virtual bool  GenerateAction(CNavAgentComponent& Agent, Game::HEntity SmartObject, Game::CActionQueueComponent& Queue, const Navigate& NavAction, const vector3& Pos) = 0;
	virtual bool  GenerateAction(CNavAgentComponent& Agent, Game::HEntity SmartObject, Game::CActionQueueComponent& Queue, const Navigate& NavAction, const vector3& Pos, const vector3& Dest, const vector3& NextDest) = 0;
	virtual bool  GenerateRecoveryAction(Game::CActionQueueComponent& Queue, const Navigate& NavAction, const vector3& ValidPos) = 0;
};

}
