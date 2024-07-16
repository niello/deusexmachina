#include "FormationManager.h"
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <AI/Navigation/NavAgentComponent.h>
#include <AI/Movement/SteerAction.h>

namespace DEM::Game
{

CFormationManager::CFormationManager(CGameSession& Owner)
	: _Session(Owner)
{
}
//---------------------------------------------------------------------

bool CFormationManager::Move(std::vector<HEntity> Entities, const rtm::vector4f& WorldPosition, const rtm::vector4f& Direction, bool Enqueue) const
{
	if (Entities.empty()) return false;

	auto pWorld = _Session.FindFeature<CGameWorld>();
	if (!pWorld) return false;

	//!!!for a single character control may skip direction to allow short steps without turning!
	//see character controller logic.
	//???or pass zero direction if no particular one is desired?

	// select positions from the current formation (no current = first available)
	// fix positions outside navmesh
	// issue movement commands (or return a set of desired positions and lookats?)
	//!!!if so, can use separate CFormation classes! manager is only a container for them.
	//manager can be non-engine, ingame helper class which knows about character controllers,
	//command queues etc, and formations are point in -> points out, and onlu interested in
	//a radius of an actor.

	//vector<pos+dir> out = pFormation->Resolve(count, position, direction, navmesh?)

	//!!!DBG TMP! send them into the one point, ignore direction!
	for (auto EntityID : Entities)
	{
		if (auto pQueue = pWorld->FindComponent<CActionQueueComponent>(EntityID))
		{
			if (!Enqueue) pQueue->Reset();

			if (auto pAgent = pWorld->FindComponent<AI::CNavAgentComponent>(EntityID))
				pQueue->EnqueueAction<AI::Navigate>(WorldPosition, 0.f);
			else
				pQueue->EnqueueAction<AI::Steer>(WorldPosition, WorldPosition, 0.f);
		}
	}

	return true;
}
//---------------------------------------------------------------------

}
