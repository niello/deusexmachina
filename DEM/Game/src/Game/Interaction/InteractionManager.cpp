#include "InteractionManager.h"
#include <Game/Interaction/Interaction.h>
#include <Game/Interaction/InteractionContext.h>

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

const IInteraction* CInteractionManager::FindInteraction(CStrID ID) const
{
	auto It = _Interactions.find(ID);
	return (It == _Interactions.cend()) ? nullptr : It->second.get();
}
//---------------------------------------------------------------------

bool CInteractionManager::IsAbilityAvailable(CStrID AbilityID, const std::vector<HEntity>& SelectedActors) const
{
	auto pAbility = FindAbility(AbilityID);
	if (!pAbility) return false;
	// check if actor selection supports an ability
	return false;
}
//---------------------------------------------------------------------

bool CInteractionManager::SelectAbility(CInteractionContext& Context, CStrID AbilityID)
{
	if (Context.Ability == AbilityID) return true;
	if (!IsAbilityAvailable(AbilityID, Context.SelectedActors)) return false;
	// if there is another ability and/or candidate action selected, reset them and target slots
	Context.Ability = AbilityID;
	// immediately check ability actions for auto-satisfied targets, AcceptTarget if so
	return false;
}
//---------------------------------------------------------------------

bool CInteractionManager::UpdateCandidateInteraction(CInteractionContext& Context)
{
	if (Context.Interaction)
	{
		// check if Interaction is still valid, reset if not
	}

	if (Context.Ability)
	{
		// check if Ability is still valid, reset if not
	}

	if (Context.SelectedTargetCount)
	{
		// check if selecated targets are still valid, reset if not
	}

	if (!Context.SelectedTargetCount)
	{
		// select first action in ability that accepts current target
	}

	return true;
}
//---------------------------------------------------------------------

bool CInteractionManager::AcceptTarget(CInteractionContext& Context)
{
	if (!Context.Interaction || Context.SelectedTargetCount == Context.SelectedTargets.size()) return false;

	// if no target and next action target is not auto-satisfiable (Self, SourceItem etc), exit
	// validate current target against candidate action, exit if invalid

	if (Context.SelectedTargets.empty())
	{
		//Context.SelectedTargets.resize(Context.Interaction->GetMaxTargetCount());
		Context.SelectedTargetCount = 0;
	}

	// add target to the selected list
/*
	if (auto Target = _InteractionContext.Interaction->CreateTarget(_InteractionContext.SelectedTargetCount))
	{
		_InteractionContext.SelectedTargets[_InteractionContext.SelectedTargetCount] = Target;
		++_InteractionContext.SelectedTargetCount;
		if (_InteractionContext.SelectedTargetCount == _InteractionContext.SelectedTargets.size())
			_InteractionContext.Interaction->Execute();
	}
*/

	return false;
}
//---------------------------------------------------------------------

bool CInteractionManager::ExecuteInteraction(CInteractionContext& Context, bool Enqueue)
{
	return false;
}
//---------------------------------------------------------------------

}
