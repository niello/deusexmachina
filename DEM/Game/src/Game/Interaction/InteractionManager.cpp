#include "InteractionManager.h"
#include <Game/Interaction/Action.h>

namespace DEM::Game
{

CInteractionManager::CInteractionManager() = default;
CInteractionManager::~CInteractionManager() = default;

bool CInteractionManager::RegisterAbility(CStrID ID, CAbility&& Ability)
{
	auto It = _Abilities.find(ID);
	if (It == _Abilities.cend())
		_Abilities.emplace(ID, std::move(Ability));
	else
		It->second = std::move(Ability);

	return true;
}
//---------------------------------------------------------------------

bool CInteractionManager::RegisterAction(CStrID ID, PAction&& Action)
{
	auto It = _Actions.find(ID);
	if (It == _Actions.cend())
		_Actions.emplace(ID, std::move(Action));
	else
		It->second = std::move(Action);

	return true;
}
//---------------------------------------------------------------------

const CAbility* CInteractionManager::FindAbility(CStrID ID) const
{
	auto It = _Abilities.find(ID);
	return (It == _Abilities.cend()) ? nullptr : &It->second;
}
//---------------------------------------------------------------------

const CAction* CInteractionManager::FindAction(CStrID ID) const
{
	auto It = _Actions.find(ID);
	return (It == _Actions.cend()) ? nullptr : It->second.get();
}
//---------------------------------------------------------------------

}
