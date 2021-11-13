#pragma once
#include <functional>
#include <memory>

// A signal that notifies an arbitrary number of callable handlers (slots).
// Implements an Observer pattern. Use it for system decoupling, e.g. for an event system.

// TODO: void Subscribe(F f, TID ID) - subscribe by ID value, can share between different slots, no connection tracking
// TODO: invoke with result accumulation, template Callable accumulator.
//       TAcc Accum(TAcc Curr, TRet SlotRet)? or void Accum(TAcc& Acc, TRet SlotRet).

namespace DEM::Events
{

struct CConnectionRecordBase
{
	// TODO: strong and weak counters for intrusive?
	uint16_t ConnectionCount = 0;
	bool     Connected = false; //???can use one byte from strong counter if 32 bits will be used?
};

class CConnection
{
protected:

	std::weak_ptr<CConnectionRecordBase> _Record;

public:

	CConnection() noexcept = default;

	CConnection(std::weak_ptr<CConnectionRecordBase>&& Record) noexcept
		: _Record(std::move(Record))
	{
		if (auto SharedRecord = _Record.lock())
			++SharedRecord->ConnectionCount;
	}

	CConnection(const CConnection& Other) noexcept
		: _Record(Other._Record)
	{
		if (auto SharedRecord = _Record.lock())
			++SharedRecord->ConnectionCount;
	}

	CConnection(CConnection&& Other) noexcept
		: _Record(std::move(Other._Record))
	{
		Other._Record.reset();
	}

	~CConnection()
	{
		// Check if the last connection was disconnected
		//???only CScopedConnection must auto-disconnect?
		const auto SharedRecord = _Record.lock();
		if (SharedRecord && --SharedRecord->ConnectionCount == 0)
			SharedRecord->Connected = false;
	}

	CConnection& operator =(const CConnection& Other) noexcept
	{
		if (this == &Other) return *this;

		const auto OldSharedRecord = _Record.lock();
		const auto NewSharedRecord = Other._Record.lock();
		if (OldSharedRecord == NewSharedRecord) return *this;

		if (NewSharedRecord) ++NewSharedRecord->ConnectionCount;

		if (OldSharedRecord && --OldSharedRecord->ConnectionCount == 0)
			OldSharedRecord->Connected = false;

		_Record = Other._Record;

		return *this;
	}

	CConnection& operator =(CConnection&& Other) noexcept
	{
		if (this == &Other) return *this;

		const auto OldSharedRecord = _Record.lock();
		if (OldSharedRecord != Other._Record.lock())
		{
			if (OldSharedRecord && --OldSharedRecord->ConnectionCount == 0)
				OldSharedRecord->Connected = false;

			_Record = std::move(Other._Record);
		}

		Other._Record.reset();

		return *this;
	}

	bool IsConnected() const
	{
		const auto SharedRecord = _Record.lock();
		return SharedRecord && SharedRecord->Connected;
	}

	void Disconnect()
	{
		if (auto SharedRecord = _Record.lock())
		{
			SharedRecord->Connected = false;
			--SharedRecord->ConnectionCount;
		}
		_Record.reset();
	}
};

template<typename T>
class CSignal;

template<typename TRet, typename... TArgs>
class CSignal<TRet(TArgs...)> final
{
protected:

	struct CNode;
	using TSlot = std::function<TRet(TArgs...)>;
	using PNode = std::shared_ptr<CNode>;

	struct CNode : public CConnectionRecordBase
	{
		TSlot Slot;
		PNode Next;
	};

	PNode Slots;
	PNode Referenced;
	PNode Pool;

	void PrepareNode()
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

		// Try to find and reuse disconnected nodes from Slots
		// TODO PERF: if this is slow on long lists can stop on first N found nodes (and remember start pos for next GC?)
		//???CollectGarbage(count N)?!
		if (!Pool) CollectGarbage();

		PNode Node;
		if (Pool)
		{
			Node = std::move(Pool);
			Pool = std::move(Node->Next);
		}
		else
			Node = std::make_shared<CNode>();

		Node->Connected = true;

		Node->Next = std::move(Slots);
		Slots = std::move(Node);
	}

	void CollectNode(PNode& Node)
	{
		// Extract the node from its current chain
		PNode FreeNode = std::move(Node);
		Node = std::move(FreeNode->Next);

		FreeNode->Slot = nullptr;

		// Attach extracted node to the free list as a new head
		PNode& Dest = FreeNode->ConnectionCount ? Referenced : Pool;
		FreeNode->Next = std::move(Dest);
		Dest = std::move(FreeNode);
	}

public:

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

		ReleaseMemory();
	}

	template<typename F>
	[[nodiscard]] CConnection Subscribe(F f)
	{
		PrepareNode();
		Slots->Slot = std::move(f);
		return CConnection(Slots);
	}

	template<typename F>
	void SubscribeAndForget(F f)
	{
		PrepareNode();
		Slots->Slot = std::move(f);
	}

	void UnsubscribeAll()
	{
		while (Slots)
		{
			Slots->Connected = false;
			CollectNode(Slots);
		}
	}

	void ReleaseMemory()
	{
		while (Referenced)
			Referenced = std::move(Referenced->Next);

		while (Pool)
			Pool = std::move(Pool->Next);
	}

	void CollectGarbage()
	{
		CNode* pCurr = Slots.get();
		CNode* pPrev = nullptr;
		while (pCurr)
		{
			if (!pCurr->Connected)
			{
				auto& SharedCurr = pPrev ? pPrev->Next : Slots;
				CollectNode(SharedCurr);
				pCurr = SharedCurr.get();
			}
			else
			{
				pPrev = pCurr;
				pCurr = pCurr->Next.get();
			}
		}
	}

	void operator()(TArgs... Args) const
	{
		//???hold strong ref? what is necessary to protect invoked list and node from destructive changes?
		const CNode* pNode = Slots.get();
		while (pNode)
		{
			if (pNode->Connected)
				pNode->Slot(Args...);
			pNode = pNode->Next.get();
		}
	}

	// operator() const + CollectGarbage() merged into a single pass for speed-up
	void operator()(TArgs... Args)
	{
		//???hold strong ref? what is necessary to protect invoked list and node from destructive changes?
		CNode* pCurr = Slots.get();
		CNode* pPrev = nullptr;
		while (pCurr)
		{
			if (!pCurr->Connected)
			{
				auto& SharedCurr = pPrev ? pPrev->Next : Slots;
				CollectNode(SharedCurr);
				pCurr = SharedCurr.get();
			}
			else
			{
				pCurr->Slot(Args...);

				pPrev = pCurr;
				pCurr = pCurr->Next.get();
			}
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
