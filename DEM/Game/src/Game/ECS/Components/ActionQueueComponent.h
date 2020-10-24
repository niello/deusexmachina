#pragma once
#include <Events/EventBase.h>
#include <Data/Metadata.h>
#include <deque>

// Queue of actions the entity must execute. Execution is performed by systems and may involve
// different components to store intermediate info. If action is not supported by an entity,
// it will remain in a queue indefinitely unless some system will clean it up. Current action
// is popped from the queue front and becomes a stack root. Action may be decomposed by intermediate
// planners. In this case a nested action will be added to the stack on top of its parent. Root action
// and all its children are considered active at the same time. Processing is controlled by systems.

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

// Like an opaque iterator, but allows to check validity without access to the collection
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
	EActionStatus                   _Status = EActionStatus::NotQueued;

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

	// Universal remove (with state? how to remove root correctly?)
	// Enqueue and PushSubAction are different

	template<typename T, typename... TArgs>
	T* EnqueueAction(TArgs&&... Args)
	{
		static_assert(std::is_base_of_v<Events::CEventBase, T>, "All entity actions must be derived from CEventBase");

		auto pAction = EnqueueAction(std::make_unique<T>(std::forward<TArgs>(Args)...));
		return pAction ? static_cast<T*>(pAction) : nullptr;
	}

	// DequeueAction() - need status or must be set on finalization? Dequeue is only for top-level queue users, e.g. QueueSystem.

	Events::CEventBase* FindActive(Events::CEventID ID) const;

	template<typename... T>
	HAction FindActive(HAction From = {}) const
	{
		auto It = From ? (++std::reverse_iterator(ItFromHandle(From))) : _Stack.crbegin();

		// Walk from nested sub-actions to the stack root
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

	HAction GetParent(HAction Handle) const
	{
		if (!Handle) return {};
		auto It = ItFromHandle(Handle);
		return (It == _Stack.cbegin()) ? HAction{} : HandleFromIt(--It);
	}

	HAction GetRoot() const
	{
		return _Stack.empty() ? HAction{} : HandleFromIt(_Stack.cbegin());
	}

	EActionStatus GetStatus(HAction Handle) const
	{
		if (!Handle) return EActionStatus::NotQueued;
		if (Handle._It == &_Stack.back()) return _Status;
		return EActionStatus::Active;
	}

	void SetStatus(HAction Handle, EActionStatus Status)
	{
		if (!Handle || Status == EActionStatus::New || Status == EActionStatus::NotQueued) return;
		_Stack.erase(++ItFromHandle(Handle), _Stack.cend()); // Pop children
		_Status = Status;
	}

	template<typename T, typename... TArgs>
	HAction PushChild(HAction Parent, TArgs&&... Args)
	{
		static_assert(std::is_base_of_v<Events::CEventBase, T>, "All entity actions must be derived from CEventBase");

		if (!Parent) return {};

		auto ParentIt = ItFromHandle(Parent);

		// Try to find child of the same type, make it an immediate child of the Parent and protect from popping
		auto ChildIt = ++ParentIt;
		if (ChildIt->get()->GetID() == T::RTTI)
		{
			++ChildIt;
		}
		else for (auto It = ChildIt + 1; It != _Stack.cend(); ++It)
		{
			if (It->get()->GetID() == T::RTTI)
			{
				std::swap(*ChildIt, *It);
				++ChildIt;
				break;
			}
		}

		// Pop remaining children
		_Stack.erase(ChildIt, _Stack.cend());

		if (ParentIt != (--_Stack.cend()))
		{
			// Reuse found child of the requested type
			T* pReused = static_cast<T*>(_Stack.back().get());
			pReused->~T();
			n_placement_new(pReused, T(std::forward<TArgs>(Args)...));
		}
		else
		{
			// Create new child
			_Stack.push_back(std::make_unique<T>(std::forward<TArgs>(Args)...));
		}

		_Status = EActionStatus::New;
		return HAction(&_Stack.back());
	}

	void Reset()
	{
		_Stack.clear();
		_Queue.clear();
		_Status = EActionStatus::NotQueued; //???cancelled?
	}

	//=====================================================================

	Events::CEventBase* EnqueueAction(Events::PEventBase&& Action);
	Events::CEventBase* GetActiveStackTop() const { return _Stack.empty() ? nullptr : _Stack.back().get(); }
	bool                FinalizeActiveAction(const Events::CEventBase& Action, EActionStatus Result);
	bool                RemoveAction(const Events::CEventBase& Action);

	//!!!FIXME: REDESIGN! When modify existing but already finished action, must reactivate it.
	void FIXME_ReactivateAction() { if (!_Stack.empty()) _Status = EActionStatus::Active; }
	//!!!FIXME: REDESIGN! Now it is a hack, but it is used for waiting without dedicated Wait action.
	void FIXME_PopSubActions(const Events::CEventBase& Action);

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
