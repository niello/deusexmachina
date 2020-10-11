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

	Events::PEventBase  PrepareToPushSubAction(Events::CEventID ID, const Events::CEventBase& Parent);

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
	Events::CEventBase* GetImmediateSubAction(const Events::CEventBase& Parent) const;
	void                ActivateStack() { if (!_Stack.empty() && _Status == EActionStatus::New) _Status = EActionStatus::Active; }
	bool                FinalizeActiveAction(const Events::CEventBase& Action, EActionStatus Result);
	bool                ClearActiveStack() { if (!_Stack.empty()) RemoveAction(*_Stack.front()); }
	bool                RemoveAction(const Events::CEventBase& Action);

	//!!!FIXME: REDESIGN! When modify existing but already finished action, must reactivate it.
	void FIXME_ReactivateAction() { if (!_Stack.empty()) _Status = EActionStatus::Active; }
	//!!!FIXME: REDESIGN! Now it is a hack, but it is used for waiting without dedicated Wait action.
	void FIXME_PopSubActions(const Events::CEventBase& Action);

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

	template<typename T>
	T* GetImmediateSubAction(const Events::CEventBase& Parent) const
	{
		static_assert(std::is_base_of_v<Events::CEventBase, T>, "All entity actions must be derived from CEventBase");

		auto pAction = GetImmediateSubAction(Parent);
		return pAction ? pAction->As<T>() : nullptr;
	}

	template<typename T, typename... TArgs>
	T* PushSubActionForParent(const Events::CEventBase& Parent, TArgs&&... Args)
	{
		static_assert(std::is_base_of_v<Events::CEventBase, T>, "All entity actions must be derived from CEventBase");

		if (_Stack.empty()) return nullptr; //???or push to root?

		if (auto FreeObject = PrepareToPushSubAction(T::RTTI, Parent))
		{
			//if constexpr (sizeof...(TArgs) > 0)
			{
				// Recreate object in place
				T* pReused = static_cast<T*>(FreeObject.get());
				pReused->~T();
				n_placement_new(pReused, T(std::forward<TArgs>(Args)...));
			}
			_Stack.push_back(std::move(FreeObject));
		}
		else
		{
			_Stack.push_back(std::make_unique<T>(std::forward<TArgs>(Args)...));
		}

		_Status = EActionStatus::New;

		return static_cast<T*>(_Stack.back().get());
	}

	template<typename T, typename... TArgs>
	T* PushSubActionOnTop(TArgs&&... Args)
	{
		static_assert(std::is_base_of_v<Events::CEventBase, T>, "All entity actions must be derived from CEventBase");

		if (_Stack.empty()) return nullptr; //???or push to root?
		_Stack.push_back(std::make_unique<T>(std::forward<TArgs>(Args)...));
		_Status = EActionStatus::New;
		return static_cast<T*>(_Stack.back().get());
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
