#pragma once
#include <StdDEM.h>

// Simple pool for fast allocation of multiple frequently created/destroyed objects, typically small.
// CPoolAllocator allows to mix any number of types as long as all of them have sizeof <= ObjectByteSize.
// CPool is a wrapper for a single type pool.

template <UPTR ObjectByteSize, UPTR ObjectsPerChunk = 128>
class CPoolAllocator
{
	static_assert(ObjectByteSize > 0 && ObjectsPerChunk > 0);

protected:

	union CRecord
	{
		U8       Object[ObjectByteSize];
		CRecord* pNext;
	};

	struct CChunkNode
	{
		std::unique_ptr<CChunkNode> Next;
		CRecord                     ChunkRecords[ObjectsPerChunk];
	};

	std::unique_ptr<CChunkNode> Chunks;
	CRecord*                    pFreeRecords = nullptr;

#ifdef _DEBUG
	UPTR       CurrAllocatedCount = 0;
#endif

public:

	~CPoolAllocator() { Clear(); }

	void* Allocate()
	{
		void* pAllocatedRec = nullptr;

		if (pFreeRecords)
		{
			pAllocatedRec = pFreeRecords->Object;
			pFreeRecords = pFreeRecords->pNext;
		}
		else
		{
			auto NewChunk = std::make_unique<CChunkNode>();
			NewChunk->Next = std::move(Chunks);
			Chunks = std::move(NewChunk);

			pAllocatedRec = Chunks->ChunkRecords->Object;

			// We had already taken the first record, add remaining ones to the free list
			if constexpr (ObjectsPerChunk > 1)
			{
				pFreeRecords = Chunks->ChunkRecords + 1;
				CRecord* pEnd = Chunks->ChunkRecords + ObjectsPerChunk - 1;
				for (auto pCurr = pFreeRecords; pCurr < pEnd; ++pCurr)
					pCurr->pNext = pCurr + 1;
				pEnd->pNext = nullptr;
			}
		}

#ifdef _DEBUG
		++CurrAllocatedCount;
#endif

		return pAllocatedRec;
	}

	void Free(void* pAllocatedRec)
	{
		if (!pAllocatedRec) return;

#ifdef _DEBUG
		--CurrAllocatedCount;
#endif

		((CRecord*)pAllocatedRec)->pNext = pFreeRecords;
		pFreeRecords = (CRecord*)pAllocatedRec;
	}

	void Clear()
	{
#ifdef _DEBUG
		if (CurrAllocatedCount > 0)
		{
			::Sys::Error("Pool reports %d unreleased records\n", CurrAllocatedCount);
			CurrAllocatedCount = 0;
		}
#endif

		Chunks = nullptr;
		pFreeRecords = nullptr;
	}

	template<typename T, typename... TArgs> T* Construct(TArgs&&... Args)
	{
		static_assert(sizeof(T) <= ObjectByteSize);
		auto pNew = static_cast<T*>(Allocate());
		n_placement_new(pNew, T)(std::forward<TArgs>(Args)...);
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

template<typename T, UPTR ObjectsPerChunk = 128>
class CPool : public CPoolAllocator<sizeof(T), ObjectsPerChunk>
{
public:

	using CPoolAllocator::Construct;
	using CPoolAllocator::Destroy;

	template<typename... TArgs> T* Construct(TArgs&&... Args) { return Construct<T, TArgs...>(std::forward<TArgs>(Args)...); }
	void Destroy(T* pPtr)  { Destroy<T>(pPtr); }
};
