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
	uint16_t ConnectionCount = 0; //???TODO: could check existense of weak_ptrs instead?

	virtual bool IsConnected() const noexcept = 0;
	virtual void Disconnect() = 0;
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
			SharedRecord->Disconnect();
	}

	CConnection& operator =(const CConnection& Other) noexcept
	{
		if (this == &Other) return *this;

		const auto OldSharedRecord = _Record.lock();
		const auto NewSharedRecord = Other._Record.lock();
		if (OldSharedRecord == NewSharedRecord) return *this;

		if (NewSharedRecord) ++NewSharedRecord->ConnectionCount;

		if (OldSharedRecord && --OldSharedRecord->ConnectionCount == 0)
			OldSharedRecord->Disconnect();

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
				OldSharedRecord->Disconnect();

			_Record = std::move(Other._Record);
		}

		Other._Record.reset();

		return *this;
	}

	bool IsConnected() const noexcept
	{
		const auto SharedRecord = _Record.lock();
		return SharedRecord && SharedRecord->IsConnected();
	}

	void Disconnect()
	{
		if (auto SharedRecord = _Record.lock())
		{
			--SharedRecord->ConnectionCount;
			SharedRecord->Disconnect();
		}
		_Record.reset();
	}

	operator bool() const noexcept { return IsConnected(); }
};

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

		Node->Next = std::move(Slots);
		Slots = std::move(Node);
	}

	void CollectNode(PNode& Node)
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
		Slots->Connected = true;
		return CConnection(Slots);
	}

	template<typename F>
	void SubscribeAndForget(F f)
	{
		PrepareNode();
		Slots->Slot = std::move(f);
		Slots->Connected = true;
	}

	void UnsubscribeAll()
	{
		while (Slots) CollectNode(Slots);
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
			if (pCurr->Connected)
			{
				pPrev = pCurr;
				pCurr = pCurr->Next.get();
			}
			else
			{
				auto& SharedCurr = pPrev ? pPrev->Next : Slots;
				CollectNode(SharedCurr);
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
			CollectNode(SharedCurr);
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
