#pragma once
#include <Game/Interaction/Ability.h>
#include <Data/StringID.h>
#include <map>

// Stores all actions and abilities available in the current game.
// External systems access them by ID.

namespace DEM::Game
{
using PAction = std::unique_ptr<class CAction>;

class CInteractionManager
{
protected:

	std::map<CStrID, CAbility> _Abilities;
	std::map<CStrID, PAction>  _Actions;

public:

	CInteractionManager();
	~CInteractionManager();

	bool            RegisterAbility(CStrID ID, CAbility&& Ability);
	bool            RegisterAction(CStrID ID, PAction&& Action);

	const CAbility* FindAbility(CStrID ID) const;
	const CAction*  FindAction(CStrID ID) const;
};

}
