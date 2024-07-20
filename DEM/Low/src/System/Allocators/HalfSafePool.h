#pragma once
#include <StdDEM.h>
#include <atomic>

// A variation of CPoolAllocator that supports freeing objects from multiple threads.
// Allocations and clearing are allowed only from the same thread. Used for allocating
// jobs in a work-stealing scheduler.

// TODO: rename Half -> Semi

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
	CRecord*                    _pFreeRecords = nullptr;     // Two lists are used to reduce contention, see Allocate()
	std::atomic<CRecord*>       _pDisposedRecords = nullptr;

#ifdef _DEBUG
	std::atomic<UPTR>           _CurrAllocatedCount = 0;
#endif

public:

#ifdef _DEBUG
	~CHalfSafePoolAllocator()
	{
		const auto CurrAllocatedCount = _CurrAllocatedCount.load();
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

		// Try acquiring a record from a newly allocated block. The block is allocated in the same thread
		// so there is no contention for _pFreeRecords and all dependent memory effects are already observable.
		if (auto pFreeRecords = _pFreeRecords)
		{
			_pFreeRecords = pFreeRecords->pNext;
			return pFreeRecords->Object;
		}

		// Try acquiring a free (deallocated) record. Load-consume pDisposedRecords->* by address dependency.
		// There is no concurrent Allocate() possible so if there is a free record now there is a guarantee
		// that there will be a free record later, concurrent Free() may only add more of them.
		auto pDisposedRecords = _pDisposedRecords.load(std::memory_order_consume);

		// There is neither free nor disposed record, allocate a new chunk
		if (!pDisposedRecords)
		{
			auto NewChunk = std::make_unique<CChunkNode>();
			NewChunk->Next = std::move(_Chunks);
			_Chunks = std::move(NewChunk);

			// Take the first record of the new chunk
			void* pAllocatedRec = _Chunks->ChunkRecords->Object;

			// Add a chain of remaining new records to the free list
			if constexpr (ObjectsPerChunk > 1)
			{
				_pFreeRecords = _Chunks->ChunkRecords + 1;
				CRecord* pEnd = _Chunks->ChunkRecords + ObjectsPerChunk - 1;
				for (auto pCurr = _pFreeRecords; pCurr < pEnd; ++pCurr)
					pCurr->pNext = pCurr + 1;
				pEnd->pNext = nullptr;
			}

			return pAllocatedRec;
		}

		// A technical loop, no more than 2 iterations possible. See the comment in 'else' branch.
		while (true)
		{
			if (auto pRecord = pDisposedRecords->pNext)
			{
				// Concurret Free() calls access only _pDisposedRecords. A chain starting at its pNext is not shared
				// and can be detached without synchronization, leaving the system in a consistent state.
				_pFreeRecords = pRecord->pNext;
				pDisposedRecords->pNext = nullptr;
				return pRecord->Object;
			}
			else
			{
				// The only free record is a head of disposed records. We contend for it with Free() calls and must use CAS.
				// If the CAS is succeeded, the head is ours and we acquire it for allocation. If the CAS is failed it means
				// that a concurrent Free() has added a new record to _pDisposedRecords, and this in turn means that the new
				// pDisposedRecords is guaranteed to have non-null pNext. Then we allow the loop to proceed to 'if' block.
				if (_pDisposedRecords.compare_exchange_strong(pDisposedRecords, nullptr, std::memory_order_consume, std::memory_order_consume))
					return pDisposedRecords->Object;
			}
		}

		// Can never be reached
		return nullptr;
	}

	// Call from any thread
	void Free(void* pObject)
	{
		if (!pObject) return;

#ifdef _DEBUG
		// TODO: add optional debug validation of incoming pointer
		_CurrAllocatedCount.fetch_sub(1, std::memory_order_relaxed);
#endif

		// Store-release pNext, it will be load-consumed in Allocate()
		auto pAllocatedRec = std::launder(reinterpret_cast<CRecord*>(pObject));
		pAllocatedRec->pNext = _pDisposedRecords.load(std::memory_order_relaxed);
		do {} while (!_pDisposedRecords.compare_exchange_weak(pAllocatedRec->pNext, pAllocatedRec, std::memory_order_release, std::memory_order_relaxed));
	}

	// Call from the owner thread only
	void Clear()
	{
#ifdef _DEBUG
		// Intentionally clear the pool. Allocated pointers become invalid, but no warning issued.
		_CurrAllocatedCount.store(0, std::memory_order_relaxed);
#endif

		_Chunks = nullptr;
		_pDisposedRecords.store(nullptr, std::memory_order_relaxed);
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
