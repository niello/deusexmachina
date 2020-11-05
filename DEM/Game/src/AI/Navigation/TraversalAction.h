#pragma once
#include <Core/Object.h>
#include <Game/ECS/Entity.h>
#include <Math/Vector3.h>

// Controls a path edge traversal by providing appropriate action for the agent,
// e.g. Steer (Walk), Swim, Jump, OpenDoor, ClimbLadder etc.

namespace DEM::Game
{
	class CGameWorld;
	class CActionQueueComponent;
	class HAction;
}

namespace DEM::AI
{
using PTraversalAction = Ptr<class CTraversalAction>;
struct CNavAgentComponent;
class Navigate;

class CTraversalAction : public ::Core::CObject
{
	RTTI_CLASS_DECL(CTraversalAction, ::Core::CObject);

public:

	virtual float GetSqTriggerRadius(float AgentRadius, float OffmeshTriggerRadius) const = 0;
	virtual bool  CanStartTraversingOffmesh(Game::CGameWorld& World, CNavAgentComponent& Agent, Game::HEntity Controller, const vector3& Pos) const { return true; }
	virtual bool  CanEndTraversingOffmesh(Game::CGameWorld& World, CNavAgentComponent& Agent, Game::HEntity Controller, const vector3& Pos) const { return true; }
	virtual bool  NeedSlowdownBeforeStart(CNavAgentComponent& Agent) const { return true; }
	virtual bool  GenerateAction(Game::CGameWorld& World, CNavAgentComponent& Agent, Game::HEntity Controller, Game::CActionQueueComponent& Queue, Game::HAction NavAction, const vector3& Pos) = 0;
};

}
