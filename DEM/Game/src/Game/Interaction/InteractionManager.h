#pragma once
#include <Game/Interaction/Ability.h>
#include <Game/ECS/Entity.h>
#include <map>

// Handles interaction of the player player with the game world. All available
// interactions and abilities available in the current game.
// External systems access them by ID.

namespace DEM::Game
{
using PInteraction = std::unique_ptr<class IInteraction>;
struct CInteractionContext;

class CInteractionManager
{
protected:

	std::map<CStrID, CAbility>     _Abilities;
	std::map<CStrID, PInteraction> _Interactions;

public:

	CInteractionManager();
	~CInteractionManager();

	bool                RegisterAbility(CStrID ID, CAbility&& Ability);
	bool                RegisterInteraction(CStrID ID, PInteraction&& Interaction);

	const CAbility*     FindAbility(CStrID ID) const;
	const IInteraction* FindAction(CStrID ID) const;

	bool                IsAbilityAvailable(CStrID AbilityID, const std::vector<HEntity>& SelectedActors) const;
	bool                SelectAbility(CInteractionContext& Context, CStrID AbilityID);
	bool                UpdateCandidateInteraction(CInteractionContext& Context);
	bool                AcceptTarget(CInteractionContext& Context);
	// Revert
	bool                ExecuteInteraction(CInteractionContext& Context, bool Enqueue);
};

}
