#pragma once
#include <functional>
#include <memory>

// A signal that notifies an arbitrary number of callable handlers (slots).
// Implements an Observer pattern. Use it for system decoupling, e.g. for an event system.

namespace DEM
{
template<typename T>
class CSignal;

template<typename TRet, typename... TArgs>
class CSignal<TRet(TArgs...)> final
{
protected:

	struct CNode;
	using TSlot = std::function<TRet(TArgs...)>;
	using PNode = std::shared_ptr<CNode>;

	struct CNode
	{
		// TODO: strong and weak counters for intrusive
		TSlot Slot;
		PNode Next;
		bool  Connected = false;
	};

	PNode Slots;
	PNode Referenced;
	PNode Pool;

public:

	CSignal() = default;
	CSignal(const CSignal&) = delete;
	CSignal(CSignal&&) noexcept = default;
	CSignal& operator =(const CSignal&) = delete;
	CSignal& operator =(CSignal&&) noexcept = default;
	~CSignal() = default;

	template<typename F>
	void Subscribe(F f)
	{
		// TODO: get from cache. If empty, scan Referenced and fill the pool from it. If no luck, make_shared.
		PNode Node = std::make_shared<CNode>();
		Node->Slot = std::move(f);

		Node->Next = std::move(Slots);
		Slots = std::move(Node);
	}

	void UnsubscribeAll()
	{
		// unsubscribe in a loop
	}

	void operator()(TArgs... Args) const
	{
		//???hold strong ref? what is necessary to protect invoked list from destructive changes?
		const CNode* pNode = Slots.get();
		while (pNode)
		{
			pNode->Slot(Args...);
			pNode = pNode->Next.get();
		}
	}

	bool Empty() const
	{
		const CNode* pNode = Slots.get();
		while (pNode)
		{
			if (pNode->Connected) return false;
			pNode = pNode->Next.get();
		}
		return true;
	}

	//???remove disconnected here? what if called during the invocation?
	size_t GetSlotCount() const
	{
		size_t Count = 0;
		const CNode* pNode = Slots.get();
		while (pNode)
		{
			if (pNode->Connected) ++Count;
			pNode = pNode->Next.get();
		}
		return Count;
	}
};

}
