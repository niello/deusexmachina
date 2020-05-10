#pragma once
#include <Events/EventBase.h>
#include <Data/Metadata.h>
#include <deque>

// Queue of actions the entity must execute. Execution is performed by systems and may involve
// different components to store intermediate info. If action is not supported by an entity,
// it will remain in a queue indefinitely unless some system will clean it up.
// Action execution may lead to decomposition and additional planning. In this case a nested
// action will be added to the stack on top of its parent, the current action. Root action
// itself remains at the Queue front and is not moved to the Stack. All actions in the stack
// and the root one are considered active at the same time, flow is controlled by systems.

namespace DEM::Game
{

struct CActionQueueComponent
{
	std::deque<Events::PEventBase>  Queue;
	std::vector<Events::PEventBase> Stack;

	// execution status of the most nested current action

	CActionQueueComponent() = default;
	CActionQueueComponent(const CActionQueueComponent& Other) = delete;
	CActionQueueComponent& operator =(const CActionQueueComponent& Other) = delete;
	CActionQueueComponent(CActionQueueComponent&& Other) = default;
	CActionQueueComponent& operator =(CActionQueueComponent&& Other) = default;

	Events::CEventBase* FindActive(Events::CEventID ID)
	{
		// Start from the most nested sub-action of the current action
		for (auto It = Stack.rbegin(); It != Stack.rend(); ++It)
			if ((*It)->GetID() == ID)
				return (*It).get();

		// No matching sub-action found, try the current root action
		return (Queue.empty() || Queue.front()->GetID() != ID) ? nullptr : Queue.front().get();
	}

	template<typename T>
	T* FindActive()
	{
		static_assert(std::is_base_of_v<Events::CEventBase, T>, "All entity actions must be derived from CEventBase");

		auto pAction = FindActive(T::RTTI);
		return pAction ? static_cast<T*>(pAction) : nullptr;
	}

	Events::CEventBase* RequestSubAction(Events::CEventID ID, const Events::CEventBase& Parent)
	{
		// Find parent
		auto It = std::find_if(Stack.begin(), Stack.end(), [&Parent](const auto& Elm)
		{
			return Elm.get() == &Parent;
		});
		if (It == Stack.cend()) return nullptr;

		// Check its sub-action, if exists
		++It;
		if (It == Stack.cend()) return nullptr;

		// Sub-action is not of requested type, cancel the whole sub-stack of the parent
		if ((*It)->GetID() != ID)
		{
			Stack.erase(It, Stack.end());
			return nullptr;
		}

		return It->get();
	}

	template<typename T>
	T* RequestSubAction(const Events::CEventBase& Parent)
	{
		static_assert(std::is_base_of_v<Events::CEventBase, T>, "All entity actions must be derived from CEventBase");

		auto pAction = RequestSubAction(T::RTTI, Parent);
		return pAction ? static_cast<T*>(pAction) : nullptr;
	}
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<Game::CActionQueueComponent>() { return "DEM::Game::CActionQueueComponent"; }
template<> inline constexpr auto RegisterMembers<Game::CActionQueueComponent>()
{
	return std::make_tuple
	(
	);
}

}
