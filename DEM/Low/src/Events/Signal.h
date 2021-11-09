#pragma once
#include <functional>
#include <memory>

// A signal that notifies an arbitrary number of callable handlers (slots).
// Implements an Observer pattern. Use it for system decoupling, e.g. for an event system.

namespace DEM
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

	bool IsConnected() const
	{
		const auto SharedRecord = _Record.lock();
		return SharedRecord && SharedRecord->Connected;
	}

	void Disconnect()
	{
		if (auto SharedRecord = _Record.lock())
			SharedRecord->Connected = false;
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
						pLastNotReferenced->Next = std::move(Pool);
						auto& First = pLastReferenced ? pLastReferenced->Next : Referenced;
						Pool = std::move(First);
						First = std::move(pPrev->Next);
						pLastNotReferenced = nullptr;
					}
				}
				else
				{
					// Track a chunk of not referenced nodes
					if (!pLastReferenced) pLastReferenced = pPrev;
					pLastNotReferenced = pCurr;
				}

				pPrev = pCurr;
				pCurr = pCurr->Next.get();
			}

			// Move the last tracked chunk to the pool
			if (pLastNotReferenced)
			{
				pLastNotReferenced->Next = std::move(Pool);
				auto& First = pLastReferenced ? pLastReferenced->Next : Referenced;
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
			Node = std::make_shared<CNode>();

		Node->Connected = true;

		Node->Next = std::move(Slots);
		Slots = std::move(Node);
	}

	void CollectNode(CNode* pPrev, PNode Node)
	{
		Node->Slot = nullptr;

		auto& Broken = pPrev ? pPrev->Next : Slots;
		auto& Dest = Node->ConnectionCount ? Referenced : Pool;
		Broken = std::move(Node->Next);
		Node->Next = std::move(Dest);
		Dest = std::move(Node);
	}

public:

	CSignal() = default;
	CSignal(const CSignal&) = delete;
	CSignal(CSignal&&) noexcept = default;
	CSignal& operator =(const CSignal&) = delete;
	CSignal& operator =(CSignal&&) noexcept = default;
	~CSignal() = default;

	template<typename F>
	CConnection Subscribe(F f)
	{
		PrepareNode();
		Slots->Slot = std::move(f);
		return CConnection(Slots);
	}

	void UnsubscribeAll()
	{
		while (Slots)
		{
			pCurr->Connected = false;
			CollectNode(nullptr, Slots);
		}
	}

	void ReleaseMemory()
	{
		Referenced = nullptr;
		Pool = nullptr;
	}

	void operator()(TArgs... Args) const
	{
		//???hold strong ref? what is necessary to protect invoked list from destructive changes?
		const CNode* pNode = Slots.get();
		while (pNode)
		{
			if (pNode->Connected)
				pNode->Slot(Args...);
			pNode = pNode->Next.get();
		}
	}

	// Performs garbage collection
	//???need also an explicit GC call?
	void operator()(TArgs... Args)
	{
		//???hold strong ref? what is necessary to protect invoked list from destructive changes?
		const CNode* pNode = Slots.get();
		while (pNode)
		{
			if (pNode->Connected)
				pNode->Slot(Args...);
			// else GC (can modify list right now? what about multithreading?)
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
