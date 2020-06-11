#pragma once
#include <Game/Interaction/Ability.h>
#include <map>

// Stores all interactions and abilities available in the current game.
// External systems access them by ID.

namespace DEM::Game
{
using PInteraction = std::unique_ptr<class IInteraction>;

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
};

}
