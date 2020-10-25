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

	auto Get() const { return _It ? _It->get() : nullptr; }

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

	template<typename T, typename... TNext>
	inline bool IsActionOneOf(Events::CEventID ID) const
	{
		static_assert(std::is_base_of_v<Events::CEventBase, T>, "All entity actions must be derived from CEventBase");
		if (ID == T::RTTI) return true;
		if constexpr (sizeof...(TNext) > 0) return IsActionOneOf<TNext...>(ID);
		return false;
	}
	//---------------------------------------------------------------------

	// NB: handle must be valid, no additional checks performed
	inline auto ItFromHandle(HAction Handle) const { return _Stack.begin() + (Handle._It - _Stack.data()); }
	//---------------------------------------------------------------------

	static inline HAction HandleFromIt(std::vector<Events::PEventBase>::const_iterator It) { return HAction{ &(*It) }; }
	//---------------------------------------------------------------------

	// ResetOnReuse - when false and immediate child is reused, its state and children are not reset
	template<typename T, bool ResetOnReuse, typename... TArgs>
	HAction PushChildT(HAction Parent, TArgs&&... Args)
	{
		static_assert(std::is_base_of_v<Events::CEventBase, T>, "All entity actions must be derived from CEventBase");

		if (!Parent) return {};

		const auto ParentIt = ItFromHandle(Parent);

		bool Reset = true;

		// Try to find child of the same type, make it an immediate child of the Parent and protect from popping
		auto ChildIt = ParentIt + 1;
		for (auto It = ChildIt; It != _Stack.cend(); ++It)
		{
			if (It->get()->GetID() == T::RTTI)
			{
				// Low-level vector usage optimization, ignore iterator constantness
				if (ChildIt != It)
					std::swap(const_cast<Events::PEventBase&>(*ChildIt), const_cast<Events::PEventBase&>(*It));
				else if constexpr (!ResetOnReuse)
					Reset = false;
				++ChildIt;
				break;
			}
		}

		if (Reset)
		{
			// Pop remaining children, mark reused one as new
			_Stack.erase(ChildIt, _Stack.cend());
			_Status = EActionStatus::New;
		}
		else
		{
			// Reactivate reused action if it is the most nested and was already finished
			if (ChildIt == (--_Stack.cend()) && _Status != EActionStatus::New)
				_Status = EActionStatus::Active;
		}

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

		return HAction(&_Stack.back());
	}
	//---------------------------------------------------------------------

public:

	CActionQueueComponent() = default;
	CActionQueueComponent(const CActionQueueComponent& Other) = delete;
	CActionQueueComponent(CActionQueueComponent&& Other) = default;
	CActionQueueComponent& operator =(const CActionQueueComponent& Other) = delete;
	CActionQueueComponent& operator =(CActionQueueComponent&& Other) = default;
	~CActionQueueComponent() = default;

	//???small pool allocator for events? one for all components? one for all small things? can be static field, ensure MT safety

	template<typename T, typename... TArgs>
	void EnqueueAction(TArgs&&... Args)
	{
		static_assert(std::is_base_of_v<Events::CEventBase, T>, "All entity actions must be derived from CEventBase");
		_Queue.push_back(std::make_unique<T>(std::forward<TArgs>(Args)...));
	}
	//---------------------------------------------------------------------

	void RunNextAction()
	{
		_Stack.clear();
		if (_Queue.empty())
		{
			_Status = EActionStatus::NotQueued;
		}
		else
		{
			_Stack.push_back(std::move(_Queue.front()));
			_Queue.pop_front();
			_Status = EActionStatus::New;
		}
	}
	//---------------------------------------------------------------------

	void Reset(EActionStatus Status = EActionStatus::NotQueued)
	{
		_Stack.clear();
		_Queue.clear();
		_Status = Status;
	}
	//---------------------------------------------------------------------

	HAction FindActive(Events::CEventID ID, HAction From = {}) const
	{
		auto It = _Stack.crbegin();
		if (From)
		{
			It = std::reverse_iterator(ItFromHandle(From));
			if (It == _Stack.crend()) return {};
			++It;
		}

		// Walk from nested sub-actions to the stack root
		for (; It != _Stack.crend(); ++It)
			if ((*It)->GetID() == ID) // Could also check if it is a parametrized event and skip other checks if so
				return HandleFromIt((++It).base());

		return {};
	}
	//---------------------------------------------------------------------

	template<typename... T>
	HAction FindActive(HAction From = {}) const
	{
		auto It = _Stack.crbegin();
		if (From)
		{
			It = std::reverse_iterator(ItFromHandle(From));
			if (It == _Stack.crend()) return {};
			++It;
		}

		// Walk from nested sub-actions to the stack root
		for (; It != _Stack.crend(); ++It)
			if (IsActionOneOf<T...>((*It)->GetID())) // Could also check if it is a parametrized event and skip other checks if so
				return HandleFromIt((++It).base());

		return {};
	}
	//---------------------------------------------------------------------

	HAction GetChild(HAction Handle) const
	{
		if (!Handle) return {};
		auto It = ++ItFromHandle(Handle);
		return (It == _Stack.cend()) ? HAction{} : HandleFromIt(It);
	}
	//---------------------------------------------------------------------

	HAction GetParent(HAction Handle) const
	{
		if (!Handle) return {};
		auto It = ItFromHandle(Handle);
		return (It == _Stack.cbegin()) ? HAction{} : HandleFromIt(--It);
	}
	//---------------------------------------------------------------------

	HAction GetRoot() const { return _Stack.empty() ? HAction{} : HandleFromIt(_Stack.cbegin()); }
	//---------------------------------------------------------------------

	size_t GetStackDepth() const { return _Stack.size(); }
	//---------------------------------------------------------------------

	size_t GetQueueSize() const { return _Queue.size() + (_Stack.empty() ? 0 : 1); }
	//---------------------------------------------------------------------

	EActionStatus GetStatus(HAction Handle) const
	{
		if (!Handle) return EActionStatus::NotQueued;
		if (Handle._It == &_Stack.back()) return _Status;
		return EActionStatus::Active;
	}
	//---------------------------------------------------------------------

	// NB: pops child actions
	void SetStatus(HAction Handle, EActionStatus Status)
	{
		if (!Handle || Status == EActionStatus::New || Status == EActionStatus::NotQueued) return;
		_Stack.erase(++ItFromHandle(Handle), _Stack.cend()); // Pop children
		_Status = Status;
	}
	//---------------------------------------------------------------------

	template<typename T, typename... TArgs>
	inline HAction PushOrUpdateChild(HAction Parent, TArgs&&... Args)
	{
		return PushChildT<T, false, TArgs...>(Parent, std::forward<TArgs>(Args)...);
	}
	//---------------------------------------------------------------------

	template<typename T, typename... TArgs>
	inline HAction PushChild(HAction Parent, TArgs&&... Args)
	{
		return PushChildT<T, true, TArgs...>(Parent, std::forward<TArgs>(Args)...);
	}
	//---------------------------------------------------------------------
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
