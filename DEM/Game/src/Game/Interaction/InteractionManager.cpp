#include "InteractionManager.h"
#include <Game/Interaction/Interaction.h>

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

bool CInteractionManager::RegisterInteraction(CStrID ID, PInteraction&& Interaction)
{
	auto It = _Interactions.find(ID);
	if (It == _Interactions.cend())
		_Interactions.emplace(ID, std::move(Interaction));
	else
		It->second = std::move(Interaction);

	return true;
}
//---------------------------------------------------------------------

const CAbility* CInteractionManager::FindAbility(CStrID ID) const
{
	auto It = _Abilities.find(ID);
	return (It == _Abilities.cend()) ? nullptr : &It->second;
}
//---------------------------------------------------------------------

const IInteraction* CInteractionManager::FindAction(CStrID ID) const
{
	auto It = _Interactions.find(ID);
	return (It == _Interactions.cend()) ? nullptr : It->second.get();
}
//---------------------------------------------------------------------

bool CInteractionManager::SelectAbility(CInteractionContext& Context, CStrID AbilityID)
{
	// if not changed, exit
	// if no ability with ID, fail
	// check if selected actors support an ability, fail if not?
	// if there is another ability and/or candidate action selected, reset them and target slots
	// immediately check ability actions for auto-satisfied targets, AcceptTarget if so
	return false;
}
//---------------------------------------------------------------------

bool CInteractionManager::UpdateCandidateInteraction(CInteractionContext& Context)
{
	// if already has selected action, check if it is still valid, reset if not
	// if already has selected ability, check if it is still valid, reset if not
	// if there are targets selected, check if they are still valid, reset if not
	// if there are no targets selected, select first action in ability that accepts current target
	return false;

/*
	if (_InteractionContext.pAbility)
	{
		if (_InteractionContext.pInteraction)
		{
			//if action became invalid, _InteractionContext.pAction = nullptr;
		}

		//if (!_InteractionContext.pAction)
		//	_InteractionContext.pAction = _InteractionContext.pAbility->ChooseAction();
	}
*/
}
//---------------------------------------------------------------------

bool CInteractionManager::AcceptTarget(CInteractionContext& Context)
{
	// if no candidate action, exit
	// if no target and next action target is not auto-satisfiable (Self, SourceItem etc), exit
	// if max targets selected
	// validate current target against candidate action, exit if invalid
	// if no target slots allocated, allocate for candidate action
	// add target to the selected list
	return false;

/*
	if (_InteractionContext.SelectedTargets.empty())
	{
		_InteractionContext.SelectedTargets.resize(_InteractionContext.Interaction->GetMaxTargetCount());
		_InteractionContext.SelectedTargetCount = 0;
	}

	if (auto Target = _InteractionContext.Interaction->CreateTarget(_InteractionContext.SelectedTargetCount))
	{
		_InteractionContext.SelectedTargets[_InteractionContext.SelectedTargetCount] = Target;
		++_InteractionContext.SelectedTargetCount;
		if (_InteractionContext.SelectedTargetCount == _InteractionContext.SelectedTargets.size())
			_InteractionContext.Interaction->Execute();
	}
*/
}
//---------------------------------------------------------------------

}
