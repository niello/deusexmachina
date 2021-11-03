#pragma once
#include <functional>
#include <memory>

// A signal that notifies an arbitrary number of callable handlers (slots).
// Implements an Observer pattern. Use it for system decoupling, e.g. for an event system.

namespace DEM
{

struct CConnectionRecordBase
{
	// TODO: strong and weak counters for intrusive
	bool Connected = false; //???can use one byte from strong counter?
};

class CConnection
{
protected:

	std::weak_ptr<CConnectionRecordBase> _Record;

public:

	CConnection(std::weak_ptr<CConnectionRecordBase>&& Record) : _Record(std::move(Record)) {}
	~CConnection()
	{
		// FIXME: std weak pointer has no access to the weak counter, can't unsubscribe here
		//!!!Also Referenced / Pool scheme will not work if we don't know the weak count!
		// if (_Record.weak_use_count() == 1) Disconnect();
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
		// TODO: get from cache. If empty, scan Referenced and fill the pool from it. If no luck, make_shared.
		PNode Node = std::make_shared<CNode>();
		Node->Slot = std::move(f);
		Node->Connected = true;

		CConnection Conn(Node);

		Node->Next = std::move(Slots);
		Slots = std::move(Node);

		return Conn;
	}

	void UnsubscribeAll()
	{
		// unsubscribe in a loop - GC each node
	}

	//!!!need a cached memory cleanup call - free Referenced and Pool!

	//???garbage collection only in non-const version? could be a mean for the user to control GC.
	//???need also an explicit GC call?
	void operator()(TArgs... Args) const
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
