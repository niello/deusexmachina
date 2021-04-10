#pragma once
#include <Game/Interaction/TargetFilter.h>
#include <Game/Interaction/InteractionContext.h>
#include <Objects/LockComponent.h>
#include <Game/ECS/GameWorld.h>

// Accepts only locked objects

// TODO: templated HasComponent target filter? Name-based too? Also use generic scripted filter?

namespace DEM::RPG
{

class CLockedTargetFilter : public Game::ITargetFilter
{
public:

	virtual bool IsTargetValid(Game::CGameSession& Session, const Game::CInteractionContext& Context, U32 Index) const override
	{
		// Check for the lock component
		const auto& Target = (Index == CURRENT_TARGET) ? Context.CandidateTarget : Context.Targets[Index];
		if (!Target.Valid) return false;
		auto pWorld = Session.FindFeature<Game::CGameWorld>();
		return pWorld && pWorld->FindComponent<CLockComponent>(Target.Entity);
	}
};

}
