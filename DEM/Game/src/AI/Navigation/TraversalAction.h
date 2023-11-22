#pragma once
#include <Core/Object.h>
#include <Game/ECS/Entity.h>
#include <rtm/vector4f.h>

// Controls a path edge traversal by providing appropriate action for the agent,
// e.g. Steer (Walk), Swim, Jump, OpenDoor, ClimbLadder etc.

namespace DEM::Game
{
	class CGameSession;
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
	virtual bool  CanStartTraversingOffmesh(Game::CGameWorld& World, CNavAgentComponent& Agent, Game::HEntity Controller, const rtm::vector4f& Pos) const { return true; }
	virtual bool  CanEndTraversingOffmesh(Game::CGameWorld& World, CNavAgentComponent& Agent, Game::HEntity Controller, const rtm::vector4f& Pos) const { return true; }
	virtual bool  NeedSlowdownBeforeStart(CNavAgentComponent& Agent) const { return true; }
	virtual bool  GenerateAction(Game::CGameSession& Session, CNavAgentComponent& Agent, Game::HEntity Actor, Game::HEntity Controller, Game::CActionQueueComponent& Queue, Game::HAction NavAction, const rtm::vector4f& Pos) = 0;
};

}
