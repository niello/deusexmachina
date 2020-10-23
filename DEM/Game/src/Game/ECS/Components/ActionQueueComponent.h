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
	Cancelled,
	NotQueued
};

class HAction final
{
private:

	friend class CActionQueueComponent;

	const Events::PEventBase* _It = nullptr;

	HAction(const Events::PEventBase* It) : _It(It) {}

public:

	HAction() = default;

	operator bool() const { return !!_It; }

	template<class T>
	T* As() const
	{
		static_assert(std::is_base_of_v<Events::CEventBase, T>, "All entity actions must be derived from CEventBase");
		return (_It && _It->get()) ? (*_It)->As<T>() : nullptr;
	}
};

class CActionQueueComponent final
{
protected:

	std::vector<Events::PEventBase> _Stack;
	std::deque<Events::PEventBase>  _Queue;
	EActionStatus                   _Status = EActionStatus::Succeeded;

	Events::PEventBase  PrepareToPushSubAction(Events::CEventID ID, const Events::CEventBase& Parent);

	template<typename T, typename... TNext>
	inline bool IsActionOneOf(Events::CEventID ID) const
	{
		static_assert(std::is_base_of_v<Events::CEventBase, T>, "All entity actions must be derived from CEventBase");
		if (ID == T::RTTI) return true;
		if constexpr (sizeof...(TNext) > 0) return IsActionOneOf<TNext...>(ID);
		return false;
	}

	// NB: handle must be valid, no additional checks performed
	inline auto ItFromHandle(HAction Handle) const { return _Stack.begin() + (Handle._It - _Stack.data()); }

	static inline HAction HandleFromIt(std::vector<Events::PEventBase>::const_iterator It) { return HAction{ &(*It) }; }

public:

	CActionQueueComponent() = default;
	CActionQueueComponent(const CActionQueueComponent& Other) = delete;
	CActionQueueComponent(CActionQueueComponent&& Other) = default;
	CActionQueueComponent& operator =(const CActionQueueComponent& Other) = delete;
	CActionQueueComponent& operator =(CActionQueueComponent&& Other) = default;
	~CActionQueueComponent() = default;

	//???small pool allocator for events? one for all components? one for all small things? can be static field, ensure MT safety

	// Push new
	// Push or update existing (top only? by handle? search here or always provide target handle)
	// Remove by handle with all children. If root, requires terminal state, otherwise allows InProgress. Never allows New state?
	// ClearChildren, possibly with setting state
	//???state is set for popped action or for current action on the stack top?

	// Go from nested sub-actions to the stack root
	template<typename... T>
	HAction FindActive(HAction From = {}) const
	{
		auto It = From ? (++std::reverse_iterator(ItFromHandle(From))) : _Stack.crbegin();

		for (; It != _Stack.crend(); ++It)
			if (IsActionOneOf<T...>((*It)->GetID())) // Could also check if it is a parametrized event and skip other checks if so
				return HandleFromIt((++It).base());

		return {};
	}

	HAction GetChild(HAction Handle) const
	{
		if (!Handle) return {};
		auto It = ++ItFromHandle(Handle);
		return (It == _Stack.cend()) ? HAction{} : HandleFromIt(It);
	}

	EActionStatus GetStatus(HAction Handle) const
	{
		if (!Handle) return EActionStatus::NotQueued;
		if (Handle._It == &_Stack.back()) return _Status;
		return EActionStatus::Active;
	}

	//=====================================================================

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
