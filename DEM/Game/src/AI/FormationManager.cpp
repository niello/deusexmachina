#include "FormationManager.h"
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <AI/CommandQueueComponent.h>
#include <AI/Navigation/NavAgentComponent.h>
#include <AI/Movement/SteerAction.h>

namespace DEM::Game
{

CFormationManager::CFormationManager(CGameSession& Owner)
	: _Session(Owner)
{
}
//---------------------------------------------------------------------

bool CFormationManager::Move(rtm::vector4f_arg0 WorldPosition, rtm::vector4f_arg1 Direction, const std::vector<HEntity>& Entities,
	std::vector<AI::CCommandFuture>* pOutCommands, bool Enqueue) const
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

	if (pOutCommands)
	{
		pOutCommands->clear();
		pOutCommands->reserve(Entities.size());
	}

	//!!!DBG TMP! now sends them into the one point and ignores direction! Must be smarter!
	for (auto EntityID : Entities)
	{
		auto* pQueue = pWorld->FindOrAddComponent<AI::CCommandQueueComponent>(EntityID);
		if (!Enqueue) pQueue->Reset();

		AI::CCommandFuture Future;
		if (auto pAgent = pWorld->FindComponent<const AI::CNavAgentComponent>(EntityID))
			Future = pQueue->EnqueueCommand<AI::Navigate>(WorldPosition, 0.f);
		else
			Future = pQueue->EnqueueCommand<AI::Steer>(WorldPosition, WorldPosition, 0.f);

		if (pOutCommands)
			pOutCommands->push_back(std::move(Future));
	}

	return true;
}
//---------------------------------------------------------------------

}
