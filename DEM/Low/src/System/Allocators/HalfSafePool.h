#pragma once
#include <StdDEM.h>
#include <atomic>

// A variation of CHalfSafePoolAllocator that supports freeing objects from multiple threads.
// Allocations and clearing are allowed only from the same thread. Used for allocating
// jobs in a work-stealing scheduler.

template <UPTR ObjectByteSize, UPTR Alignment = 0, UPTR ObjectsPerChunk = 128>
class CHalfSafePoolAllocator final
{
	static_assert(ObjectByteSize > 0 && ObjectsPerChunk > 0);

protected:

	union CRecord
	{
		alignas(Alignment) std::byte Object[ObjectByteSize];
		CRecord* pNext;
	};

	struct CChunkNode
	{
		std::unique_ptr<CChunkNode> Next;
		CRecord ChunkRecords[ObjectsPerChunk];
	};

	std::unique_ptr<CChunkNode> _Chunks;
	std::atomic<CRecord*>       _pFreeRecords = nullptr;

#ifdef _DEBUG
	std::atomic<UPTR>           _CurrAllocatedCount = 0;
#endif

public:

#ifdef _DEBUG
	~CHalfSafePoolAllocator()
	{
		if (CurrAllocatedCount > 0)
			::Sys::Error("~CHalfSafePoolAllocator() > %d unreleased records\n", CurrAllocatedCount);
	}
#endif

	// Call from the owner thread only
	void* Allocate()
	{
#ifdef _DEBUG
		// The method can't fail, increment now in a single place
		_CurrAllocatedCount.fetch_add(1, std::memory_order_relaxed);
#endif

		// Try acquiring a free record. Load-consume pFreeRecords->* by address dependency.
		// Failure of compare_exchange_weak must not be stronger than success, so consume on success too.
		// NB: in a half-safe implementation we only consume changes from Free() because Allocate() can't
		// be called from a different thread and its effects are always observable in the next Allocate().
		// NB: in a half-safe implementation pFreeRecords can't become nullptr after the first check because
		// concurrent Free() may only add records and not borrow them. Full safety would require "while (pFreeRecords)".
		if (auto pFreeRecords = _pFreeRecords.load(std::memory_order_consume))
		{
			do {} while (!_pFreeRecords.compare_exchange_weak(pFreeRecords, pFreeRecords->pNext, std::memory_order_consume, std::memory_order_consume));
			return pFreeRecords->Object;
		}

		// There is no free record, must allocate a new chunk
		// NB: this is not thread safe in the current implementation, that's why the pool is "half-safe"
		auto NewChunk = std::make_unique<CChunkNode>();
		NewChunk->Next = std::move(_Chunks);
		_Chunks = std::move(NewChunk);

		// Take the first record of the new chunk
		void* pAllocatedRec = _Chunks->ChunkRecords->Object;

		// Add remaining records to the free list
		if constexpr (ObjectsPerChunk > 1)
		{
			// Build a chain of new records
			CRecord* pBegin = _Chunks->ChunkRecords + 1;
			CRecord* pEnd = _Chunks->ChunkRecords + ObjectsPerChunk - 1;
			for (auto pCurr = pBegin; pCurr < pEnd; ++pCurr)
				pCurr->pNext = pCurr + 1;

			// Atomically attach new free records to existing. Note that pFreeRecords might change
			// because some records could be freed while we were allocating a chunk and building a chain.
			// NB: would need store-release to publish new chunk and chain links from above and below, but in the
			// half-safe implementation these are read only in the same thread and are guaranteed to be observed.
			auto pFreeRecords = _pFreeRecords.load(std::memory_order_relaxed);
			do
			{
				pEnd->pNext = pFreeRecords;
			}
			while (!_pFreeRecords.compare_exchange_weak(pFreeRecords, pBegin, std::memory_order_relaxed, std::memory_order_relaxed));
		}

		return pAllocatedRec;
	}

	// Call from any thread
	void Free(void* pObject)
	{
		if (!pObject) return;

#ifdef _DEBUG
		// TODO: add optional debug validation of incoming pointer
		_CurrAllocatedCount.fetch_sub(1, std::memory_order_relaxed);
#endif

		auto pAllocatedRec = std::launder(reinterpret_cast<CRecord*>(pObject));

		// Store-release pNext, it will be load-consumed in Allocate()
		auto pFreeRecords = _pFreeRecords.load(std::memory_order_relaxed);
		do
		{
			pAllocatedRec->pNext = pFreeRecords;
		}
		while (!_pFreeRecords.compare_exchange_weak(pFreeRecords, pAllocatedRec, std::memory_order_release, std::memory_order_relaxed));
	}

	// Call from the owner thread only
	void Clear()
	{
#ifdef _DEBUG
		// Intentionally clear the pool. Allocated pointers become invalid, but no warning issued.
		_CurrAllocatedCount.store(0, std::memory_order_relaxed);
#endif

		_Chunks = nullptr;
		_pFreeRecords.store(nullptr, std::memory_order_relaxed);
	}

	// Call from the owner thread only
	template<typename T, typename... TArgs> T* Construct(TArgs&&... Args)
	{
		static_assert(sizeof(T) <= ObjectByteSize);
		auto pNew = std::launder(reinterpret_cast<T*>(Allocate()));
		new (pNew) T(std::forward<TArgs>(Args)...);
		return pNew;
	}

	// Call from any thread
	template<typename T> void Destroy(T* pPtr)
	{
		static_assert(sizeof(T) <= ObjectByteSize);
		if (pPtr)
		{
			pPtr->~T();
			Free(pPtr);
		}
	}
};

template<typename T, UPTR Alignment = alignof(T), UPTR ObjectsPerChunk = 128>
class CHalfSafePool final
{
private:

	static_assert(Alignment >= alignof(T), "Can't use alignment that is less than the object itself requires");

	CHalfSafePoolAllocator<sizeof(T), Alignment, ObjectsPerChunk> _Allocator;

public:

	template<typename... TArgs> T* Construct(TArgs&&... Args) { return _Allocator.Construct<T, TArgs...>(std::forward<TArgs>(Args)...); }
	void Destroy(T* pPtr)  { _Allocator.Destroy<T>(pPtr); }
	void Clear() { _Allocator.Clear(); }
};
