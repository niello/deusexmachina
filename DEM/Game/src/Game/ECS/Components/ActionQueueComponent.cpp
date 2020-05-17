#pragma once
#include "ActionQueueComponent.h"

namespace DEM::Game
{

void CActionQueueComponent::Reset()
{
	_Stack.clear();
	_Queue.clear();
	_Status = EActionStatus::Succeeded;
}
//---------------------------------------------------------------------

Events::CEventBase* CActionQueueComponent::EnqueueAction(Events::PEventBase&& Action)
{
	if (!Action) return nullptr;

	if (_Stack.empty())
	{
		n_assert_dbg(_Queue.empty());
		_Stack.push_back(std::move(Action));
		_Status = EActionStatus::New;
		return _Stack.back().get();
	}
	else
	{
		_Queue.push_back(std::move(Action));
		return _Queue.back().get();
	}
}
//---------------------------------------------------------------------

Events::CEventBase* CActionQueueComponent::FindActive(Events::CEventID ID) const
{
	// Start from the most nested sub-action
	for (auto It = _Stack.rbegin(); It != _Stack.rend(); ++It)
		if ((*It)->GetID() == ID)
			return (*It).get();

	return nullptr;
}
//---------------------------------------------------------------------

bool CActionQueueComponent::RemoveAction(const Events::CEventBase& Action, EActionStatus Reason)
{
	if (Queue.empty()) return false;

	if (Queue.front().get() == &Action)
	{
		Stack.clear();
		Queue.pop_front();
		Status = Reason;
		return true;
	}

	auto It = std::find_if(Stack.begin(), Stack.end(), [&Action](const auto& Elm)
	{
		return Elm.get() == &Action;
	});
	if (It == Stack.cend()) return false;

	Stack.erase(It, Stack.end());
	Status = Reason;
	return true;
}
//---------------------------------------------------------------------

bool CActionQueueComponent::RequestSubAction(Events::CEventID ID, const Events::CEventBase& Parent, Events::CEventBase*& pOutSubAction)
{
	pOutSubAction = nullptr;

	// Find parent in active actions, fail if not found
	auto It = std::find_if(Stack.begin(), Stack.end(), [&Parent](const auto& Elm)
	{
		return Elm.get() == &Parent;
	});
	if (It == Stack.cend() && Queue.front().get() != &Parent) return false;

	// If no sub-actions exist at all, exit
	if (Stack.empty()) return true;

	// Check its sub-action, if exists
	if (It == Stack.cend()) It = Stack.begin();
	else
	{
		++It;
		if (It == Stack.cend()) return true;
	}

	// Sub-action is not of requested type, cancel the whole sub-stack of the parent
	if ((*It)->GetID() != ID)
	{
		Stack.erase(It, Stack.end());
		return true;
	}

	pOutSubAction = It->get();
	return true;
}
//---------------------------------------------------------------------

}
