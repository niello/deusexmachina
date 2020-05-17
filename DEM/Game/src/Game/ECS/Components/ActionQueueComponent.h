#pragma once
#include <Events/EventBase.h>
#include <Data/Metadata.h>
#include <deque>

// Queue of actions the entity must execute. Execution is performed by systems and may involve
// different components to store intermediate info. If action is not supported by an entity,
// it will remain in a queue indefinitely unless some system will clean it up. Current action
// is popped from the queue front and becomes a current action stack root.
// Action execution may lead to decomposition and additional planning. In this case a nested
// action will be added to the stack on top of its parent, the current action. All actions in
// the stack are considered active at the same time, flow is controlled by systems.

namespace DEM::Game
{

enum class EActionStatus : U8
{
	New = 0,
	Active,
	Succeeded,
	Failed,
	Cancelled
};

class CActionQueueComponent
{
protected:

	std::vector<Events::PEventBase> _Stack;
	std::deque<Events::PEventBase>  _Queue;
	EActionStatus                   _Status = EActionStatus::Succeeded;

public:

	CActionQueueComponent() = default;
	CActionQueueComponent(const CActionQueueComponent& Other) = delete;
	CActionQueueComponent(CActionQueueComponent&& Other) = default;
	CActionQueueComponent& operator =(const CActionQueueComponent& Other) = delete;
	CActionQueueComponent& operator =(CActionQueueComponent&& Other) = default;
	~CActionQueueComponent() = default;

	EActionStatus       GetStatus() const { return _Status; }
	void                Reset();
	Events::CEventBase* EnqueueAction(Events::PEventBase&& Action);
	Events::CEventBase* FindActive(Events::CEventID ID) const;
	Events::CEventBase* GetActiveStackTop() const { return _Stack.empty() ? nullptr : _Stack.back().get(); }

	template<typename T, typename... TArgs>
	T* EnqueueAction(TArgs&&... Args)
	{
		static_assert(std::is_base_of_v<Events::CEventBase, T>, "All entity actions must be derived from CEventBase");

		auto pAction = EnqueueAction(std::make_unique<T>(std::forward<TArgs>(Args)...));
		return pAction ? static_cast<T*>(pAction) : nullptr;
	}

	template<typename T>
	T* FindActive() const
	{
		static_assert(std::is_base_of_v<Events::CEventBase, T>, "All entity actions must be derived from CEventBase");

		auto pAction = FindActive(T::RTTI);
		return pAction ? static_cast<T*>(pAction) : nullptr;
	}

	/////////////////////////////

	bool RemoveAction(const Events::CEventBase& Action, EActionStatus Reason);
	bool RequestSubAction(Events::CEventID ID, const Events::CEventBase& Parent, Events::CEventBase*& pOutSubAction);

	template<typename T>
	bool RequestSubAction(const Events::CEventBase& Parent, T*& pOutSubAction)
	{
		static_assert(std::is_base_of_v<Events::CEventBase, T>, "All entity actions must be derived from CEventBase");

		Events::CEventBase* pSubAction;
		if (!RequestSubAction(T::RTTI, Parent, pSubAction)) return false;
		pOutSubAction = pSubAction ? static_cast<T*>(pSubAction) : nullptr;
		return true;
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
