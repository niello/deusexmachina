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

Events::CEventBase* CActionQueueComponent::GetImmediateSubAction(const Events::CEventBase& Parent) const
{
	// Parent must be in active stack
	auto It = std::find_if(_Stack.begin(), _Stack.end(), [&Parent](const auto& Elm)
	{
		return Elm.get() == &Parent;
	});
	if (It == _Stack.cend()) return nullptr;
	++It;
	return (It == _Stack.cend()) ? nullptr : It->get();
}
//---------------------------------------------------------------------

bool CActionQueueComponent::FinalizeActiveAction(const Events::CEventBase& Action, EActionStatus Result)
{
	// Result must be a terminal status
	if (Result == EActionStatus::New || Result == EActionStatus::Active) return false;

	// Action must be in active stack
	auto It = std::find_if(_Stack.begin(), _Stack.end(), [&Action](const auto& Elm)
	{
		return Elm.get() == &Action;
	});
	if (It == _Stack.cend()) return false;

	// Cancel all sub-actions and set our (top) action status
	++It;
	_Stack.erase(It, _Stack.end());
	_Status = Result;
	return true;
}
//---------------------------------------------------------------------

// Only for root actions. To remove sub-action, call FinalizeActiveAction or PushSubActionForParent.
bool CActionQueueComponent::RemoveAction(const Events::CEventBase& Action)
{
	// If removing active root, clear the whole stack and pop the next action from the queue
	if (!_Stack.empty() && _Stack.front().get() == &Action)
	{
		_Stack.clear();
		if (!_Queue.empty())
		{
			_Stack.push_back(std::move(_Queue.front()));
			_Queue.pop_front();
			_Status = EActionStatus::New;
		}
		else if (_Status == EActionStatus::New || _Status == EActionStatus::Active)
		{
			// If result was not set yet, set success
			_Status = EActionStatus::Succeeded;
		}
		return true;
	}

	// If action is not current, try to remove from queue
	auto It = std::find_if(_Queue.begin(), _Queue.end(), [&Action](const auto& Elm)
	{
		return Elm.get() == &Action;
	});
	if (It == _Queue.cend()) return false;

	_Queue.erase(It);
	return true;
}
//---------------------------------------------------------------------

// Returns an action object for reuse, if found
Events::PEventBase CActionQueueComponent::PrepareToPushSubAction(Events::CEventID ID, const Events::CEventBase& Parent)
{
	Events::PEventBase FreeObject;

	// Parent must be in active stack
	auto SubStart = std::find_if(_Stack.begin(), _Stack.end(), [&Parent](const auto& Elm)
	{
		return Elm.get() == &Parent;
	});
	if (SubStart == _Stack.cend()) return nullptr;
	++SubStart;

	// Cancel all sub-actions of the specified parent. If a sub-action of
	// requested type is being discarded, reuse its object to avoid allocation.
	for (auto It = SubStart; It != _Stack.end(); ++It)
	{
		if (It->get()->GetID() == ID)
		{
			FreeObject = std::move(*It);
			break;
		}
	}
	_Stack.erase(SubStart, _Stack.end());

	return FreeObject;
}
//---------------------------------------------------------------------

}
