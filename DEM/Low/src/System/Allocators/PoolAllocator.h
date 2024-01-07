#pragma once
#include <StdDEM.h>

// Simple pool for fast allocation of multiple frequently created/destroyed objects, typically small.
// CPoolAllocator allows to mix any number of types as long as all of them have sizeof <= ObjectByteSize.
// CPool is a wrapper for a single type pool.

template <UPTR ObjectByteSize, UPTR Alignment = 0, UPTR ObjectsPerChunk = 128>
class CPoolAllocator final
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
	CRecord*                    _pFreeRecords = nullptr;

#ifdef _DEBUG
	UPTR                        _CurrAllocatedCount = 0;
#endif

public:

#ifdef _DEBUG
	~CPoolAllocator()
	{
		if (_CurrAllocatedCount > 0)
			::Sys::Error("~CPoolAllocator() > %d unreleased records\n", _CurrAllocatedCount);
	}
#endif

	void* Allocate()
	{
#ifdef _DEBUG
		++_CurrAllocatedCount;
#endif

		if (auto pRecord = _pFreeRecords)
		{
			_pFreeRecords = pRecord->pNext;
			return pRecord->Object;
		}

		auto NewChunk = std::make_unique<CChunkNode>();
		NewChunk->Next = std::move(_Chunks);
		_Chunks = std::move(NewChunk);

		void* pAllocatedRec = _Chunks->ChunkRecords->Object;

		// We had already taken the first record, add remaining ones to the free list
		if constexpr (ObjectsPerChunk > 1)
		{
			CRecord* pBegin = _Chunks->ChunkRecords + 1;
			CRecord* pEnd = _Chunks->ChunkRecords + ObjectsPerChunk - 1;
			for (auto pCurr = pBegin; pCurr < pEnd; ++pCurr)
				pCurr->pNext = pCurr + 1;
			pEnd->pNext = nullptr;

			_pFreeRecords = pBegin;
		}

		return pAllocatedRec;
	}

	void Free(void* pObject)
	{
		if (!pObject) return;

#ifdef _DEBUG
		// TODO: add optional debug validation of incoming pointer
		--_CurrAllocatedCount;
#endif

		auto pAllocatedRec = std::launder(reinterpret_cast<CRecord*>(pObject));
		pAllocatedRec->pNext = _pFreeRecords;
		_pFreeRecords = pAllocatedRec;
	}

	void Clear()
	{
#ifdef _DEBUG
		// Intentionally clear the pool. Allocated pointers become invalid, but no warning issued.
		_CurrAllocatedCount = 0;
#endif

		_Chunks = nullptr;
		_pFreeRecords = nullptr;
	}

	template<typename T, typename... TArgs> T* Construct(TArgs&&... Args)
	{
		static_assert(sizeof(T) <= ObjectByteSize);
		auto pNew = std::launder(reinterpret_cast<T*>(Allocate()));
		new (pNew) T(std::forward<TArgs>(Args)...);
		return pNew;
	}

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
class CPool final
{
private:

	static_assert(Alignment >= alignof(T), "Can't use alignment that is less than the object itself requires");

	CPoolAllocator<sizeof(T), Alignment, ObjectsPerChunk> _Allocator;

public:

	template<typename... TArgs> T* Construct(TArgs&&... Args) { return _Allocator.Construct<T, TArgs...>(std::forward<TArgs>(Args)...); }
	void Destroy(T* pPtr)  { _Allocator.Destroy<T>(pPtr); }
	void Clear() { _Allocator.Clear(); }
};
