#pragma once
#include <Events/Connection.h>
#include <functional>

// A signal that notifies an arbitrary number of callable handlers (slots).
// Implements an Observer pattern. Use it for system decoupling, e.g. for an event system.

// TODO: void Subscribe(F f, TID ID) - subscribe by ID value, can share between different slots, no connection tracking
// TODO: invoke with result accumulation, template Callable accumulator.
//       TAcc Accum(TAcc Curr, TRet SlotRet)? or void Accum(TAcc& Acc, TRet SlotRet).
// TODO: what is needed to support coroutine event handlers?

namespace DEM::Events
{

template<typename T>
class CSignal;

template<typename TRet, typename... TArgs>
class CSignal<TRet(TArgs...)> final
{
protected:

	struct CNode;
	using PNode = std::shared_ptr<CNode>;

	struct CNode final : public CConnectionRecordBase
	{
		std::function<TRet(TArgs...)> Slot;
		PNode Next;
		bool Connected = false; // Needed only due to VERY poor performance of "Slot != nullptr" checks, even in Release

		virtual bool IsConnected() const noexcept override { return Connected; }
		virtual void Disconnect() override { Slot = nullptr; Connected = false; }
	};

	thread_local static inline PNode Pool;
	thread_local static inline PNode Referenced;

	PNode Slots;

	static PNode AllocateNode()
	{
		// Try to reuse disconnected nodes no longer referenced by connection objects
		if (!Pool)
		{
			// Move no longer referenced disconnected nodes to the pool
			CNode* pCurr = Referenced.get();
			CNode* pPrev = nullptr;
			CNode* pLastReferenced = nullptr;
			CNode* pLastNotReferenced = nullptr;
			while (pCurr)
			{
				if (pCurr->ConnectionCount)
				{
					// A chunk of not referenced objects is interrupted. Move it to the pool.
					if (pLastNotReferenced)
					{
						PNode Curr = std::move(pLastNotReferenced->Next);
						pLastNotReferenced->Next = std::move(Pool);
						PNode& First = pLastReferenced ? pLastReferenced->Next : Referenced;
						Pool = std::move(First);
						First = std::move(Curr);
						pLastNotReferenced = nullptr;
					}
				}
				else
				{
					// Track a chunk of not referenced nodes
					if (!pLastNotReferenced) pLastReferenced = pPrev;
					pLastNotReferenced = pCurr;
				}

				pPrev = pCurr;
				pCurr = pCurr->Next.get();
			}

			// Move the last tracked chunk to the pool
			if (pLastNotReferenced)
			{
				pLastNotReferenced->Next = std::move(Pool);
				PNode& First = pLastReferenced ? pLastReferenced->Next : Referenced;
				Pool = std::move(First);
				First = nullptr;
			}
		}

		PNode Node;
		if (Pool)
		{
			Node = std::move(Pool);
			Pool = std::move(Node->Next);
		}
		else
		{
			Node = std::make_shared<CNode>();
		}

		return Node;
	}

	static void FreeNode(PNode& Node)
	{
		// Extract the node from its current chain
		PNode FreeNode = std::move(Node);
		Node = std::move(FreeNode->Next);

		FreeNode->Slot = nullptr;
		FreeNode->Connected = false;

		// Attach extracted node to the free list as a new head
		PNode& Dest = FreeNode->ConnectionCount ? Referenced : Pool;
		FreeNode->Next = std::move(Dest);
		Dest = std::move(FreeNode);
	}

public:

	static void ReleaseMemory()
	{
		while (Referenced)
			Referenced = std::move(Referenced->Next);

		while (Pool)
			Pool = std::move(Pool->Next);
	}

	CSignal() = default;
	CSignal(const CSignal&) = delete;
	CSignal(CSignal&&) noexcept = default;
	CSignal& operator =(const CSignal&) = delete;
	CSignal& operator =(CSignal&&) noexcept = default;
	~CSignal()
	{
		// NB: turn recursion into loop to prevent stack overflow when too many slots exist
		while (Slots)
			Slots = std::move(Slots->Next);
	}

	template<typename F>
	[[nodiscard]] CConnection Subscribe(F f)
	{
		auto Node = AllocateNode();
		Node->Slot = std::move(f);
		Node->Connected = true;
		Node->Next = std::move(Slots);
		Slots = std::move(Node);
		return CConnection(Slots);
	}

	template<typename F>
	void SubscribeAndForget(F f)
	{
		auto Node = AllocateNode();
		Node->Slot = std::move(f);
		Node->Connected = true;
		Node->Next = std::move(Slots);
		Slots = std::move(Node);
	}

	void UnsubscribeAll()
	{
		while (Slots) FreeNode(Slots);
	}

	void CollectGarbage()
	{
		CNode* pCurr = Slots.get();
		CNode* pPrev = nullptr;
		while (pCurr)
		{
			if (pCurr->Connected)
			{
				pPrev = pCurr;
				pCurr = pCurr->Next.get();
			}
			else
			{
				auto& SharedCurr = pPrev ? pPrev->Next : Slots;
				FreeNode(SharedCurr);
				pCurr = SharedCurr.get();
			}
		}
	}

	void operator()(TArgs... Args) const
	{
		// This may look awful at the beginning, but this is just how our signal should work.
		// Constant signal must be triggerable. We need to manipulate slots and nodes
		// during the invocation. Another way would be to declare all fields as mutable.
		// The main use case is triggering a signal stored by value in a constant class instance.
		// Note that subscribing to a constant signal is not allowed.
		const_cast<CSignal<TRet(TArgs...)>*>(this)->operator()(Args...);
	}

	void operator()(TArgs... Args)
	{
		CNode* pCurr = Slots.get();
		CNode* pPrev = nullptr;
		while (pCurr)
		{
			if (pCurr->Connected)
			{
				auto Slot = std::move(pCurr->Slot);
				Slot(Args...);
				if (pCurr->Connected)
				{
					pCurr->Slot = std::move(Slot);
					pPrev = pCurr;
					pCurr = pCurr->Next.get();
					continue;
				}
			}

			auto& SharedCurr = pPrev ? pPrev->Next : Slots;
			FreeNode(SharedCurr);
			pCurr = SharedCurr.get();
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
